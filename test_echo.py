#!/usr/bin/env python3
"""Phase 3 echo test — run AFTER 'make run-server' is already running."""

import asyncio
import websockets

async def main():
    uri = "ws://localhost:8080"
    print(f"Connecting to {uri} ...")

    async with websockets.connect(uri) as ws:
        messages = [
            "hello server",
            "phase 3 echo test",
            '{"action": "TEST", "data": {}}',   # raw JSON — server ignores shape for now
        ]

        for m in messages:
            await ws.send(m)
            reply = await ws.recv()
            status = "OK" if reply == m else "MISMATCH"
            print(f"  sent:  {m!r}")
            print(f"  echo:  {reply!r}  [{status}]")

    print("Done.")

asyncio.run(main())
