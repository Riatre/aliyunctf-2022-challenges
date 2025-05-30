from starlette.config import Config
from starlette.datastructures import Secret

config = Config(".env")

DEBUG = config("DEBUG", cast=bool, default=False)
OPENAI_API_KEY = config("OPENAI_API_KEY", cast=Secret)
OPENAI_API_PROXY = config("OPENAI_API_PROXY", cast=str, default="")
SESSION_SECRET = config("SESSION_SECRET", cast=Secret)
FLAG = config(
    "FLAG",
    cast=str,
    default="aliyunctf{congrats_thats_the_flag_see_you_Apr_22_JsHkbE97NH}",
)
FLAG_HIGH_ENTROPY_PIECE = config("FLAG_HIGH_ENTROPY_PIECE", cast=str, default="JsHkbE97NH")
MODEL_ID = config("MODEL_ID", cast=str, default="gpt-3.5-turbo")
MAX_INPUT_TOKENS = config("MAX_INPUT_TOKENS", cast=int, default=140)
MAX_OUTPUT_TOKENS = config("MAX_OUTPUT_TOKENS", cast=int, default=200)
BEST_N = config("BEST_N", cast=int, default=2)
REDIS_URL = config("REDIS_URL", cast=str, default="redis://localhost:6379/0")
FIRST_MESSAGE_CACHE_POPULARITY = config(
    "FIRST_MESSAGE_CACHE_POPULARITY", cast=int, default=100
)
CONTENT_MODERATION_ACCESS_KEY_ID = config("CONTENT_MODERATION_ACCESS_KEY_ID", cast=str)
CONTENT_MODERATION_ACCESS_KEY_SECRET = config(
    "CONTENT_MODERATION_ACCESS_KEY_SECRET", cast=Secret
)
CONTENT_MODERATION_ENDPOINT = config(
    "CONTENT_MODERATION_ENDPOINT", cast=str, default="green-cip.cn-shanghai.aliyuncs.com"
)
NVC_ACCESS_KEY_ID = config("NVC_ACCESS_KEY_ID", cast=str)
NVC_ACCESS_KEY_SECRET = config("NVC_ACCESS_KEY_SECRET", cast=Secret)
VALIDATE_TEAM_TOKEN_URL = config(
    "VALIDATE_TEAM_TOKEN_URL",
    cast=str,
    default="http://47.97.37.60/team_token_check.php",
)
VALIDATE_TEAM_TOKEN_SECRET = config("VALIDATE_TEAM_TOKEN_SECRET", cast=Secret)
REPLY_RATE_LIMIT = config("REPLY_RATE_LIMIT", cast=str, default="6 per minute, 50 per 3 hour")
