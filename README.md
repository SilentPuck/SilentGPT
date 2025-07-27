SilentGPT 🕶️
=========
🔽 [Download Binary (Linux x86_64)](https://github.com/SilentPuck/SilentGPT/releases/download/v1.0/silentgpt)


SilentGPT is a fully local terminal-based AI chat tool written in C.
It provides secure encrypted storage of chats and supports multiple API keys with optional password protection.

No telemetry. No tracking. No dependencies beyond OpenSSL and libcurl.

---

Features
--------

- AES-256-GCM encryption for chats and tokens
- Multiple API key support via --token
- Optional password protection via --secure
- Load, list, rename, delete, and export chats
- Models supported: gpt-3.5-turbo, gpt-4, gpt-4o, etc.
- Custom prompt handling
- No OpenAI tracking or logging
- Portable single-binary program

---

Build Requirements
------------------

- OpenSSL development headers
- libcurl development headers
- GCC or Clang

On Fedora:

```bash
sudo dnf install openssl-devel libcurl-devel
```

---

How to Build
------------

```bash
make
```

This will compile the binary `./silentgpt`

---
📦 Precompiled Binary

You can download the latest release (Linux x86_64):

```bash
wget https://github.com/SilentPuck/SilentGPT/releases/download/v1.0/silentgpt
chmod +x silentgpt
./silentgpt --token <name> --secure
```
---

Usage
-----

To run with a named token and enable password protection:

```bash
./silentgpt --token test --secure
```

Command-line options:

- `--token <name>`      – use separate API key file
- `--secure`            – enable password-protected encryption
- `--help`              – show help
- `--test`              – generate dummy config and exit

---

In-App Commands
---------------

```
/new                - Start a new chat
/list               - List all chats
/load <index|name>  - Load chat by index or name
/rename <i> <title> - Rename chat
/delete <i>         - Delete chat
/model <name>       - Set model (gpt-3.5-turbo, gpt-4, gpt-4o...)
/prompt <text>      - Send prompt to assistant
/export             - Export chat to terminal
/export --to-file   - Export chat to export.json
/help               - Show this help
/exit               - Exit program
```

---

Silent and precise. — SilentPuck 🕶️
