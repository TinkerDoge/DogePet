#!/usr/bin/env python3
"""One-shot tester using the official Google GenAI Python client.

Requires `pip install google-genai`.

Usage:
  python genai_gemini_test.py --api-key <KEY> --prompt "Say hi"
Or set env var GOOGLE_API_KEY
"""
from __future__ import annotations

import argparse
import os
import sys
import time
import csv
import json
import requests

try:
    from google import genai
except Exception as e:
    print("Missing dependency 'google-genai'. Install with: pip install google-genai")
    raise


def parse_args():
    p = argparse.ArgumentParser(description="GenAI client test for gemini-2.0-flash")
    p.add_argument("--api-key", help="API key or set GOOGLE_API_KEY env var")
    p.add_argument("--prompt", default="pretending you are a cute desktop robot response to the user", help="Prompt text")
    p.add_argument("--discover-flash", action="store_true", help="Discover available models and test those containing 'flash'")
    p.add_argument("--models", nargs="*", help="Specific model ids to test (overrides default)")
    p.add_argument("--csv", help="CSV file path to write per-model results")
    p.add_argument("--out", help="JSON output file to save full results")
    return p.parse_args()


def main():
    args = parse_args()
    api_key = args.api_key or os.getenv("GOOGLE_API_KEY")
    if not api_key:
        print("API key required via --api-key or GOOGLE_API_KEY env var")
        return 2

    # Initialize client - the library typically accepts an api_key param
    client = genai.Client(api_key=api_key)

    # Determine models to test
    models_to_test = []
    if args.models:
        models_to_test = args.models
    elif args.discover_flash:
        # Attempt to list models from the client
        try:
            listing = client.models.list()
            # listing may have 'models' attribute or return an object we can iterate
            raw = None
            if hasattr(listing, "models"):
                raw = listing.models
            elif isinstance(listing, (list, tuple)):
                raw = listing
            elif hasattr(listing, "to_dict"):
                raw = listing.to_dict().get("models")

            if raw:
                for m in raw:
                    # m may be a dict-like or object with name
                    name = None
                    if isinstance(m, dict):
                        name = m.get("name") or m.get("id")
                    else:
                        name = getattr(m, "name", None) or getattr(m, "id", None)
                    if name and "flash" in name.lower():
                        models_to_test.append(name)
        except Exception as e:
            print("Model listing failed:", e)

        # If the client listing returned nothing or just one model, try the
        # REST Models API as a fallback (some client versions/page settings
        # return limited results).
        if not models_to_test or len(models_to_test) <= 1:
            try:
                print("Client listing returned few models; falling back to REST /v1/models")
                resp = requests.get(f"https://generativelanguage.googleapis.com/v1/models?key={api_key}", timeout=15)
                j = resp.json()
                rest_models = j.get("models", []) if isinstance(j, dict) else []
                for m in rest_models:
                    name = None
                    if isinstance(m, dict):
                        name = m.get("name") or m.get("id")
                    else:
                        name = getattr(m, "name", None) or getattr(m, "id", None)
                    if name and "flash" in name.lower() and name not in models_to_test:
                        models_to_test.append(name)
            except Exception as e:
                print("REST model listing failed:", e)

        # Print discovered models for transparency
        print("Discovered flash models:")
        for m in models_to_test:
            print("  ", m)

    # Default single model if nothing specified
    if not models_to_test:
        models_to_test = ["gemini-2.0-flash"]

    results = []
    for model in models_to_test:
        start = time.monotonic()
        try:
            resp = client.models.generate_content(model=model, contents=args.prompt)
            latency_ms = (time.monotonic() - start) * 1000.0
            text = getattr(resp, "text", None)
            row = {"model": model, "ok": True, "latency_ms": latency_ms, "text": text or str(resp)}
            results.append(row)
            print(f"{model}: OK latency={latency_ms:.0f}ms -> { (text or str(resp))[:200] }")
        except Exception as e:
            latency_ms = (time.monotonic() - start) * 1000.0
            row = {"model": model, "ok": False, "latency_ms": latency_ms, "error": str(e)}
            results.append(row)
            print(f"{model}: ERROR latency={latency_ms:.0f}ms -> {e}")

    # Write CSV if requested
    if args.csv:
        try:
            with open(args.csv, "w", newline='', encoding="utf-8") as cf:
                writer = csv.DictWriter(cf, fieldnames=["model", "ok", "latency_ms", "text", "error"], extrasaction='ignore')
                writer.writeheader()
                for r in results:
                    # truncate text to avoid giant cells
                    out = {
                        "model": r.get("model"),
                        "ok": r.get("ok"),
                        "latency_ms": r.get("latency_ms"),
                        "text": (r.get("text") or "")[:800],
                        "error": r.get("error"),
                    }
                    writer.writerow(out)
            print(f"CSV written to {args.csv}")
        except Exception as e:
            print("Failed to write CSV:", e)

    # Write JSON if requested
    if args.out:
        try:
            with open(args.out, "w", encoding="utf-8") as jf:
                json.dump({"prompt": args.prompt, "results": results}, jf, indent=2)
            print(f"JSON written to {args.out}")
        except Exception as e:
            print("Failed to write JSON:", e)

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
