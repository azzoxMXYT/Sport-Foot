"""Sign a PE using a local PEM key, then check its signature and digest."""
import asyncio
import sys
import struct
from pathlib import Path
from winsign.crypto import load_pem_cert, load_private_key
from winsign.pefile import sign_file, pefile, calc_checksum
from winsign.sign import key_signer
from winsign.verify import verify_pefile

source, output, cert, key = map(Path, sys.argv[1:])
with source.open('rb') as src, output.open('w+b') as dst:
    asyncio.run(sign_file(src, dst, 'sha256', load_pem_cert(cert.read_bytes()),
                          key_signer(load_private_key(key.read_bytes()))))
    # PE/COFF requires each WIN_CERTIFICATE entry to end on an 8-byte boundary.
    dst.seek(0)
    hdr = pefile.parse_stream(dst).optional_header
    pad = -hdr.certtable_size % 8
    if pad:
        dst.seek(0, 2)
        dst.write(bytes(pad))
        size = struct.pack('<I', hdr.certtable_size + pad)
        for offset in (hdr.certtable_offset, hdr.certtable_info + 4):
            dst.seek(offset)
            dst.write(size)
        checksum = calc_checksum(dst, hdr.checksum_offset)
        dst.seek(hdr.checksum_offset)
        dst.write(struct.pack('<I', checksum))
    dst.seek(0)
    result = verify_pefile(dst)
    if not result:
        raise SystemExit(str(result))
    print('Authenticode SHA-256 : signature et empreinte valides.')
