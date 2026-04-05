#!/bin/sh
# blueDuck container entrypoint
set -e

# ── Validate required environment variables ────────────────────────────────
if [ -z "$BLUEDUCK_SECRET_KEY" ]; then
    echo "ERROR: BLUEDUCK_SECRET_KEY is not set."
    echo ""
    echo "  Generate a key with:  openssl rand -hex 32"
    echo "  Then add it to your .env file:  BLUEDUCK_SECRET_KEY=<key>"
    exit 1
fi

key_len=$(printf '%s' "$BLUEDUCK_SECRET_KEY" | wc -c)
if [ "$key_len" -ne 64 ]; then
    echo "ERROR: BLUEDUCK_SECRET_KEY must be exactly 64 hex characters (32 bytes)."
    echo "  Got $key_len characters."
    echo "  Generate a valid key with:  openssl rand -hex 32"
    exit 1
fi

# ── Print startup info ─────────────────────────────────────────────────────
echo "──────────────────────────────────────────"
echo " blueDuck starting"
echo " DB   : ${BD_DB_HOST:-db}:${BD_DB_PORT:-5432}/${BD_DB_NAME:-blueduck}"
echo " Port : 8080"
echo " NVD  : $([ -n "$BD_NVD_API_KEY" ] && echo 'API key set' || echo 'no API key (rate-limited)')"
echo "──────────────────────────────────────────"

exec /usr/bin/blueduck --config /etc/blueduck/config.json
