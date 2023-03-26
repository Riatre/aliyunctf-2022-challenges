FROM node:18.15.0-alpine3.16 AS jsbuild

WORKDIR /app
COPY frontend/package.json frontend/yarn.lock ./
RUN yarn install --frozen-lockfile
COPY templates/ ../templates/
COPY frontend/ .
RUN yarn build

FROM python:3.11-slim AS requirements-stage
WORKDIR /tmp
RUN pip install poetry
COPY vendor /tmp/vendor
COPY ./pyproject.toml ./poetry.lock* /tmp/
RUN poetry export -f requirements.txt --output requirements.txt --without-hashes

# Final image
FROM tiangolo/uvicorn-gunicorn-starlette:python3.11-slim
COPY --from=requirements-stage /tmp/requirements.txt /app/requirements.txt
COPY --from=requirements-stage /tmp/vendor /tmp/vendor
RUN pip install --no-cache-dir --upgrade -r /app/requirements.txt && rm -rf /tmp/vendor
COPY scripts/prestart.sh /app/
COPY templates /app/templates
COPY ws8k /app/ws8k
COPY --from=jsbuild /app/dist /app/frontend/dist
