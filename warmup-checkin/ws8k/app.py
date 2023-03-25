from starlette.applications import Starlette
from starlette.requests import Request
from starlette.responses import RedirectResponse, JSONResponse, FileResponse
from starlette.templating import Jinja2Templates
from starlette.routing import Route, Mount
from starlette.middleware import Middleware
from starlette.staticfiles import StaticFiles
from starlette_session import SessionMiddleware
import starlette_session.backends
from redis import asyncio as aioredis
import slowapi
import slowapi.util
from slowapi.errors import RateLimitExceeded
import tiktoken
import aiohttp

from . import settings, chatgpt, bot_detect

import hashlib
import logging
import time

logger = logging.getLogger(__name__)

limiter = slowapi.Limiter(
    key_func=lambda request: request.session.get("team_token_digest", None)
    or slowapi.util.get_remote_address(request),
    headers_enabled=True,
    storage_uri=settings.REDIS_URL,
)


async def index(request):
    if not request.session.get("auth", False):
        return RedirectResponse("/auth")
    return FileResponse("./templates/index.html")


@limiter.limit("3 per minute")
async def auth(request: Request):
    if request.session.get("auth", False):
        return RedirectResponse("/")
    token = request.query_params.get("token", None)
    if not token:
        return templates.TemplateResponse("login.html", {"request": request})
    async with aiohttp.ClientSession() as session:
        resp = await session.get(
            settings.VALIDATE_TEAM_TOKEN_URL,
            params={
                "team_token": token,
                "secret": str(settings.VALIDATE_TEAM_TOKEN_SECRET),
            },
        )
    j = await resp.json(content_type=None)
    if resp.status != 200 or j.get("code") != "SUCCESS":
        return templates.TemplateResponse(
            "login.html", {"request": request, "error": "Failed to validate team token"}
        )
    if not j.get("data", False):
        return templates.TemplateResponse(
            "login.html", {"request": request, "error": "Invalid team token"}
        )
    request.session["team_token_digest"] = hashlib.sha256(token.encode()).hexdigest()
    request.session["auth"] = True
    return RedirectResponse("/")


async def chat_history(request: Request):
    if not request.session.get("auth", False):
        return JSONResponse({"error": "unauthenticated"}, status_code=403)
    thread = request.session.get("chat_history", None) or []
    has_flag = any([settings.FLAG in m["content"] for m in thread])
    return JSONResponse(
        {"thread": thread, "accept_new_reply": len(thread) < 6, "solved": has_flag}
    )


@limiter.limit("6 per minute, 50 per 3 hour")
async def chat(request: Request):
    if not request.session.get("auth", False):
        return JSONResponse({"error": "unauthenticated"}, status_code=403)
    # TODO(riatre): 验证码。
    previous_messages = request.session.get("chat_history", None) or []
    if len(previous_messages) % 2 != 0:
        await new_chat(request)
        return JSONResponse({"error": "invalid state"}, status_code=400)
    if len(previous_messages) // 2 >= 3:
        return JSONResponse({"error": "nope"}, status_code=403)
    req = await request.json()
    message = req.get("msg", "")
    if not isinstance(message, str):
        return JSONResponse({"error": "invalid message"}, status_code=400)
    message = message.strip()
    if not message or len(message) > 5 * settings.MAX_INPUT_TOKENS:
        return JSONResponse({"error": "invalid message"}, status_code=400)
    # Tokenizer is CPU-heavy, kick in bot detection here.
    nvc_val = req.get("nvc", None)
    if nvc_val is None or not isinstance(nvc_val, str) or not nvc_val:
        return JSONResponse({"error": "nvc required"}, status_code=400)
    nvc_res = await bot_detect.analyze_nvc(
        nvc_val, source_ip=slowapi.util.get_remote_address(request)
    )
    if not nvc_res.okay:
        return JSONResponse(
            {"error": "nvc action needed", "nvc_code": nvc_res.code}, status_code=403
        )
    if len(app.state.tokenizer.encode(message)) > settings.MAX_INPUT_TOKENS:
        return JSONResponse({"error": "message too long"}, status_code=400)
    user_msg_timestamp = int(time.time())
    resp = await chatgpt.chat_with_ctf_assistant(
        message,
        userid=request.session["team_token_digest"],
        previous_messages=previous_messages,
    )
    assistant_msg_timestamp = int(time.time())
    request.session["chat_history"] = previous_messages + [
        {"role": "user", "content": message, "timestamp": user_msg_timestamp},
        {"role": "assistant", "content": resp, "timestamp": assistant_msg_timestamp},
    ]
    return await chat_history(request)


async def new_chat(request: Request):
    request.session["chat_history"] = None
    return await chat_history(request)


def on_startup():
    app.state.tokenizer = tiktoken.encoding_for_model(settings.MODEL_ID)


routes = [
    Route("/", endpoint=index),
    Route("/auth", endpoint=auth, methods=["GET"]),
    Route("/reply", endpoint=chat_history, methods=["GET"]),
    Route("/reply", endpoint=chat, methods=["POST"]),
    Route("/new", endpoint=new_chat, methods=["POST"]),
    Mount("/static", app=StaticFiles(directory="static"), name="static"),
]
middlewares = [
    Middleware(
        SessionMiddleware,
        secret_key=settings.SESSION_SECRET,
        cookie_name="SESSION",
        backend_type=starlette_session.backends.BackendType.aioRedis,
        backend_client=aioredis.from_url(settings.REDIS_URL),
        same_site="strict",
    )
]
templates = Jinja2Templates(directory="templates")
app = Starlette(routes=routes, middleware=middlewares, on_startup=[on_startup])
app.state.limiter = limiter
app.add_exception_handler(RateLimitExceeded, slowapi._rate_limit_exceeded_handler)
