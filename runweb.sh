#!/bin/bash
set -euo pipefail

echo "Open http://localhost:8080/"

# Serve files using a local webserver
emrun --no_browser --port 8080 public/
