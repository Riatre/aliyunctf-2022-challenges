from starlette.applications import Starlette
from starlette.requests import Request, HTTPConnection
from starlette.responses import RedirectResponse, JSONResponse
from starlette.templating import Jinja2Templates
from starlette.routing import Route, Mount
from starlette.middleware import Middleware
from starlette.staticfiles import StaticFiles
from starlette_session import SessionMiddleware
import starlette_context.plugins
from starlette_context.middleware import RawContextMiddleware
import starlette_session.backends
from redis import asyncio as aioredis
import slowapi
import slowapi.util
from slowapi.errors import RateLimitExceeded
import tiktoken
import aiohttp
import structlog

from . import settings, chatgpt, bot_detect
from .logging import setup_logging

import hashlib
import time
import json

logger = structlog.get_logger()

limiter = slowapi.Limiter(
    key_func=lambda request: request.session.get("team_token_digest", None)
    or slowapi.util.get_remote_address(request),
    headers_enabled=True,
    storage_uri=settings.REDIS_URL,
    auto_check=False,
)

if not settings.DEBUG:
    with open("frontend/dist/manifest.json") as f:
        FE_MANIFEST = json.load(f)


class TeamTokenPlugin(starlette_context.plugins.base.Plugin):
    key = "team_token"

    async def process_request(self, request: Request | HTTPConnection):
        return request.session.get("team_token_first_half", None)


class IPAddressPlugin(starlette_context.plugins.base.Plugin):
    key = "ip"

    async def process_request(self, request: Request | HTTPConnection):
        return slowapi.util.get_remote_address(request)


async def index(request: Request):
    if not request.session.get("auth", False):
        return RedirectResponse("/auth.html")

    def _asset_uri(path):
        if settings.DEBUG:
            return str(request.url.replace(path=path, port=5173))
        return f"/{FE_MANIFEST[path]['file']}"

    return templates.TemplateResponse(
        "index.html",
        {
            "request": request,
            "debug": settings.DEBUG,
            "asset_uri": _asset_uri,
        },
    )


@limiter.limit("6 per minute")
async def auth(request: Request):
    request.state.view_rate_limit = None
    if request.session.get("auth", False):
        return RedirectResponse("/")
    token = request.query_params.get("token", None)
    if not token:
        return RedirectResponse("/")
    app.state.limiter._check_request_limit(request, auth.__wrapped__, False)
    async with aiohttp.ClientSession() as session:
        resp = await session.get(
            settings.VALIDATE_TEAM_TOKEN_URL,
            params={
                "team_token": token,
                "secret": str(settings.VALIDATE_TEAM_TOKEN_SECRET),
            },
        )
    try:
        j = await resp.json(content_type=None)
    except:
        logger.exception("auth.invalid_json")
    if resp.status != 200 or j.get("code") != "SUCCESS":
        logger.error("auth.validation_fail", status=resp.status, result=j)
        return RedirectResponse("/auth.html?error=Failed to validate team token")
    if not j.get("data", False):
        logger.info("auth.fail")
        return RedirectResponse("/auth.html?error=Invalid team token")
    logger.info("auth.ok")
    request.session["team_token_first_half"] = token[: len(token) // 2]
    request.session["team_token_digest"] = hashlib.sha256(token.encode()).hexdigest()
    request.session["auth"] = True
    return RedirectResponse("/")


async def auth_page(request: Request):
    css_uri = (
        "https://unpkg.com/tailwindcss@^2/dist/tailwind.min.css"
        if settings.DEBUG
        else f"/{FE_MANIFEST['src/main.css']['file']}"
    )
    return templates.TemplateResponse(
        "login.html",
        {
            "request": request,
            "css_uri": css_uri,
            "error": request.query_params.get("error"),
        },
    )


async def chat_history(request: Request):
    if not request.session.get("auth", False):
        return JSONResponse({"error": "unauthenticated"}, status_code=403)
    thread = request.session.get("chat_history", None) or []
    has_flag = any([settings.FLAG in m["content"] for m in thread])
    if has_flag:
        logger.info("chat_history.solved", thread=thread)
    return JSONResponse(
        {"thread": thread, "accept_new_reply": len(thread) < 6, "solved": has_flag}
    )


@limiter.limit(settings.REPLY_RATE_LIMIT)
async def chat(request: Request):
    request.state.view_rate_limit = None
    if not request.session.get("auth", False):
        return JSONResponse({"error": "unauthenticated"}, status_code=403)
    previous_messages = request.session.get("chat_history", None) or []
    log = logger.bind(history=previous_messages)
    if len(previous_messages) % 2 != 0:
        await new_chat(request)
        log.warning("chat.invalid_state")
        return JSONResponse({"error": "invalid state"}, status_code=400)
    if len(previous_messages) // 2 >= 3:
        log.error("chat.too_many_replies")
        return JSONResponse({"error": "nope"}, status_code=403)
    req = await request.json()
    message = req.get("msg", "")
    if not isinstance(message, str):
        return JSONResponse({"error": "invalid message"}, status_code=400)
    message = message.strip()
    log = log.bind(message=message)
    if not message or len(message) > 10 * settings.MAX_INPUT_TOKENS:
        log.info("chat.invalid_message_too_long")
        return JSONResponse({"error": "invalid message"}, status_code=400)
    # Tokenizer is CPU-heavy, kick in bot detection here.
    nvc_val = req.get("nvc", None)
    if nvc_val is None or not isinstance(nvc_val, str) or not nvc_val:
        log.info("chat.no_nvc_sent")
        return JSONResponse({"error": "nvc required"}, status_code=400)
    nvc_res = await bot_detect.analyze_nvc(
        nvc_val, source_ip=slowapi.util.get_remote_address(request)
    )
    if not nvc_res.okay:
        logger.info("chat.nvc_fail", code=nvc_res.code)
        return JSONResponse(
            {"error": "nvc action needed", "nvc_code": nvc_res.code}, status_code=403
        )
    # Apply rate limit after NVC check.
    app.state.limiter._check_request_limit(request, chat.__wrapped__, False)
    input_token_count = len(app.state.tokenizer.encode(message))
    if input_token_count > settings.MAX_INPUT_TOKENS:
        log.info("chat.too_long_after_tokenization", tokens=input_token_count)
        return JSONResponse({"error": "message too long"}, status_code=400)
    log.info("chat.request_start")
    user_msg_timestamp = time.time()
    result = await chatgpt.chat_with_ctf_assistant(
        message,
        userid=request.session["team_token_digest"],
        previous_messages=previous_messages,
    )
    resp = result.message
    assistant_msg_timestamp = time.time()
    log.info(
        "chat.response",
        resp=resp,
        prompt_tokens=result.prompt_tokens,
        completion_tokens=result.completion_tokens,
        total_tokens=result.total_tokens,
        latency=assistant_msg_timestamp - user_msg_timestamp,
    )
    request.session["chat_history"] = previous_messages + [
        {"role": "user", "content": message, "timestamp": int(user_msg_timestamp)},
        {"role": "assistant", "content": resp, "timestamp": int(assistant_msg_timestamp)},
    ]
    return await chat_history(request)


async def new_chat(request: Request):
    request.session["chat_history"] = None
    logger.info("new_chat.done")
    return await chat_history(request)


def on_startup():
    app.state.tokenizer = tiktoken.encoding_for_model(settings.MODEL_ID)
    setup_logging()


routes = [
    Route("/", endpoint=index),
    Route("/auth", endpoint=auth, methods=["GET"]),
    Route("/auth.html", endpoint=auth_page, methods=["GET"]),
    Route("/reply", endpoint=chat_history, methods=["GET"]),
    Route("/reply", endpoint=chat, methods=["POST"]),
    Route("/new", endpoint=new_chat, methods=["POST"]),
]
if not settings.DEBUG:
    routes.append(
        Mount(
            "/assets", app=StaticFiles(directory="frontend/dist/assets"), name="assets"
        )
    )
middlewares = [
    Middleware(
        SessionMiddleware,
        secret_key=settings.SESSION_SECRET,
        cookie_name="SESSION",
        backend_type=starlette_session.backends.BackendType.aioRedis,
        backend_client=aioredis.from_url(settings.REDIS_URL),
        same_site="strict",
    ),
    Middleware(
        RawContextMiddleware,
        plugins=(
            starlette_context.plugins.RequestIdPlugin(force_new_uuid=True),
            TeamTokenPlugin(),
            IPAddressPlugin(),
        ),
    ),
]
templates = Jinja2Templates(directory="templates")
app = Starlette(routes=routes, middleware=middlewares, on_startup=[on_startup])
app.state.limiter = limiter
app.add_exception_handler(RateLimitExceeded, slowapi._rate_limit_exceeded_handler)
