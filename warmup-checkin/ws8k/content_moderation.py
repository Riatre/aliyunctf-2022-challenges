from alibabacloud_green20220302.client import Client
from alibabacloud_green20220302 import models
from alibabacloud_tea_openapi.models import Config

from . import settings

import json
import uuid
import structlog

logger = structlog.get_logger(__name__)


def _create_client():
    config = Config(
        access_key_id=settings.CONTENT_MODERATION_ACCESS_KEY_ID,
        access_key_secret=str(settings.CONTENT_MODERATION_ACCESS_KEY_SECRET),
        endpoint=settings.CONTENT_MODERATION_ENDPOINT,
        connect_timeout=3000,
        read_timeout=6000,
    )
    return Client(config)


async def is_text_safe_to_display(message: str) -> bool:
    """Returns True if the message is safe to be sent to the user."""
    client = _create_client()
    request = models.TextModerationRequest(
        "ai_art_detection",
        json.dumps(
            {
                "content": message[:600],
                "dataId": str(uuid.uuid4()),
            }
        ),
    )
    try:
        response = await client.text_moderation_async(request)
    except:
        logger.exception("content_moderation.fail")
        raise
    if response.status_code != 200 or response.body.data is None:
        logger.error("content_moderation.fail", response=response.body)
        return False
    labels = response.body.data.labels.split(",")
    if set(labels) & {
        "ad",
        "political_content",
        "contraband",
        "violence",
        "cyberbullying",
    }:
        logger.warning(
            "content_moderation.blocked",
            message=message,
            labels=response.body.data.labels,
            reason=response.body.data.reason,
        )
        return False
    return True
