#!/usr/bin/env python3
"""Convert Qt .ts translation file to .qm binary format (Qt6).

Format: 16-byte magic, then tag-based sections:
  0xa7 = Language (2 bytes)
  0x42 = Hashes (array of 8-byte entries: uint32 elf_hash, uint32 msg_offset)
  0x69 = Messages (serialized messages)
  0x88 = NumerusRules (plural forms)

Each message in Messages block:
  Tag 0x07 Context      (uint32 len + UTF-8 bytes)
  Tag 0x06 SourceText   (uint32 len + UTF-8 bytes)
  Tag 0x03 Translation  (uint32 len + UTF-16BE chars)
  Tag 0x01 End          (1 byte, no payload)
"""

import struct
import sys
import xml.etree.ElementTree as ET

# Qt6 .qm magic (16 bytes)
QM_MAGIC = bytes([0x3c, 0xb8, 0x64, 0x18, 0xca, 0xef, 0x9c, 0x95,
                  0xcd, 0x21, 0x1c, 0xbf, 0x60, 0xa1, 0xbd, 0xdd])

# Message-level tags (inside Messages block)
TAG_END = 0x01
TAG_TRANSLATION = 0x02
TAG_OBSOLETE1 = 0x04
TAG_SOURCETEXT = 0x05
TAG_CONTEXT = 0x06
TAG_COMMENT = 0x07

# Top-level tags
TAG_HASHES = 0x42
TAG_MESSAGES = 0x69
TAG_NUMERUS_RULES = 0x88
TAG_CONTEXTS = 0x2f
TAG_DEPENDENCIES = 0x96
TAG_LANGUAGE = 0xa7


def elf_hash(data: bytes) -> int:
    """ELF hash — used by Qt6 for .qm message lookup.
    Note: h=0 is reserved and becomes 1 via elfHash_finish."""
    h = 0
    for b in data:
        h = (h << 4) + b
        g = h & 0xF0000000
        if g:
            h ^= g >> 24
        h &= ~g
    if h == 0:
        h = 1
    return h & 0xFFFFFFFF


def encode_tag(tag: int, data: bytes) -> bytes:
    return struct.pack(">BI", tag, len(data)) + data


def encode_message(ctx: str, src: str, tgt: str) -> bytes:
    """Encode one message: Translation + Comment + SrcText + Context + End.

    Tag order matches lrelease output: translation first, then metadata.
    """
    parts = b""
    # Translation (UTF-16BE byte count + data)
    tgt_chars = tgt.encode("utf-16-be")
    parts += struct.pack(">BI", TAG_TRANSLATION, len(tgt_chars)) + tgt_chars
    # Comment (empty, UTF-8)
    parts += struct.pack(">BI", TAG_COMMENT, 0)
    # SourceText (UTF-8)
    src_bytes = src.encode("utf-8")
    parts += struct.pack(">BI", TAG_SOURCETEXT, len(src_bytes)) + src_bytes
    # Context (UTF-8)
    ctx_bytes = ctx.encode("utf-8")
    parts += struct.pack(">BI", TAG_CONTEXT, len(ctx_bytes)) + ctx_bytes
    # End
    parts += struct.pack("B", TAG_END)
    return parts


def ts_to_qm(ts_path: str, qm_path: str) -> None:
    tree = ET.parse(ts_path)
    root = tree.getroot()

    # Collect (context, source, translation) tuples
    messages: list[tuple[str, str, str]] = []
    for context in root.findall("context"):
        ctx_name = context.findtext("name", "")
        for message in context.findall("message"):
            source = message.findtext("source", "")
            translation = message.findtext("translation", "")
            tr_elem = message.find("translation")
            msg_type = tr_elem.get("type", "") if tr_elem is not None else ""

            if msg_type in ("vanished", "obsolete") or not translation:
                continue

            messages.append((ctx_name, source, translation))

    if not messages:
        # Write minimal valid .qm (magic only)
        with open(qm_path, "wb") as f:
            f.write(QM_MAGIC)
        print(f"Compiled {ts_path} -> {qm_path} (0 translations)")
        return

    # Build Messages block
    messages_block = b""
    hash_entries: list[tuple[int, int]] = []  # (elf_hash, offset_in_messages)

    for ctx, src, tgt in messages:
        offset = len(messages_block)
        # Hash combines source text + comment (comment is empty)
        h = elf_hash(src.encode("utf-8") + b"")
        hash_entries.append((h, offset))
        messages_block += encode_message(ctx, src, tgt)

    # Build Hashes block (sorted by hash for binary search compatibility)
    hash_entries.sort(key=lambda x: x[0])
    hashes_block = b""
    for h, off in hash_entries:
        hashes_block += struct.pack(">II", h, off)

    # Build .qm file
    qm = QM_MAGIC
    qm += encode_tag(TAG_LANGUAGE, b"en")  # default language hint
    qm += encode_tag(TAG_HASHES, hashes_block)
    qm += encode_tag(TAG_MESSAGES, messages_block)
    # NumerusRules: empty (no plural forms)
    qm += encode_tag(TAG_NUMERUS_RULES, struct.pack(">H", 0))

    with open(qm_path, "wb") as f:
        f.write(qm)

    print(f"Compiled {ts_path} -> {qm_path} ({len(messages)} translations)")


if __name__ == "__main__":
    if len(sys.argv) != 3:
        print(f"Usage: {sys.argv[0]} <input.ts> <output.qm>")
        sys.exit(1)
    ts_to_qm(sys.argv[1], sys.argv[2])
