FROM node:24-alpine
WORKDIR /app
# Coolify runs its health check with curl inside the container.
RUN apk add --no-cache curl && corepack enable
COPY package.json pnpm-lock.yaml ./
RUN pnpm install --prod --frozen-lockfile
COPY src ./src
ENV CONFIG_PATH=/config/config.json
EXPOSE 8787
USER node
CMD ["node", "--experimental-strip-types", "src/server.ts"]
