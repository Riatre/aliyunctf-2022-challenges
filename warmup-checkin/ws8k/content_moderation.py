from alibabacloud_green20220302.client import Client
from alibabacloud_green20220302 import models
from alibabacloud_tea_openapi.models import Config
from alibabacloud_tea_util.client import Client as UtilClient
from alibabacloud_tea_util import models as util_models

from . import settings

import json
import uuid
import logging

logger = logging.getLogger(__name__)


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
                "content": message,
                "dataId": str(uuid.uuid4()),
            }
        ),
    )
    response = await client.text_moderation_async(request)
    if response.status_code != 200:
        logger.error("Content moderation failed: %s", response.body)
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
            "Blocked message: %s, labels: %s, reason: %s",
            message,
            response.body.data.labels,
            response.body.data.reason,
        )
        return False
    return True
