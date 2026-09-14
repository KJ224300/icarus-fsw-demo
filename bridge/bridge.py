import asyncio
import json
import socket

import websockets


UDP_HOST = "127.0.0.1"
UDP_PORT = 5000

WS_HOST = "localhost"
WS_PORT = 8765


# Keep track of connected OpenMCT clients
clients = set()


async def websocket_handler(websocket):
    """
    Handles one OpenMCT WebSocket connection.
    """

    clients.add(websocket)

    print("OpenMCT connected!")

    try:
        # Keep the connection alive and listen for messages
        async for message in websocket:
            print("OpenMCT:", message)

    except websockets.exceptions.ConnectionClosed:
        pass

    finally:
        clients.discard(websocket)
        print("OpenMCT disconnected.")


async def udp_receiver():
    """
    Receives telemetry packets from the C OBC.
    """

    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)

    sock.bind((UDP_HOST, UDP_PORT))

    print(f"UDP receiver listening on {UDP_HOST}:{UDP_PORT}")

    loop = asyncio.get_running_loop()

    while True:

        # Wait for a UDP packet without blocking
        data, address = await loop.run_in_executor(
            None,
            sock.recvfrom,
            4096
        )

        message = data.decode("utf-8")

        print("UDP:", message)

        # Forward the telemetry to every connected OpenMCT client
        disconnected_clients = set()

        for client in clients:
            try:
                await client.send(message)

            except websockets.exceptions.ConnectionClosed:
                disconnected_clients.add(client)

        clients.difference_update(disconnected_clients)


async def main():

    print("ICARUS telemetry bridge starting...")

    # Start WebSocket server
    async with websockets.serve(
        websocket_handler,
        WS_HOST,
        WS_PORT
    ):

        print(f"WebSocket server listening on ws://{WS_HOST}:{WS_PORT}")

        # Run UDP receiver forever
        await udp_receiver()


if __name__ == "__main__":
    asyncio.run(main())
