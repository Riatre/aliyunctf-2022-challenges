from alibabacloud_afs20180112.client import Client
from alibabacloud_afs20180112 import models
from alibabacloud_tea_openapi.models import Config

from . import settings

from dataclasses import dataclass
import structlog
import json

logger = structlog.get_logger(__name__)

SCORE_JSON_STR = json.dumps(
    {
        200: "OK",
        400: "NC",
        600: "SC",
        700: "LC",
        800: "BLOCK",
    }
)

def _create_client():
    config = Config(
        access_key_id=settings.NVC_ACCESS_KEY_ID,
        access_key_secret=str(settings.NVC_ACCESS_KEY_SECRET),
        region_id="cn-beijing",
        connect_timeout=3000,
        read_timeout=6000,
    )
    return Client(config)


@dataclass
class AnalyzeNVCResult:
    okay: bool
    code: int


async def analyze_nvc(nvc: str, *, source_ip: str) -> AnalyzeNVCResult:
    client = _create_client()
    request = models.AnalyzeNvcRequest(source_ip, SCORE_JSON_STR, nvc)
    try:
        response = await client.analyze_nvc_async(request)
    except:
        logger.exception("nvc.analyze_fail", nvc=nvc)
        raise
    code = int(response.body.biz_code)
    return AnalyzeNVCResult(code in (100, 200), code)
