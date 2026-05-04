import hashlib

def calculate_sabre_hash(metadata_str: str, image_bytes: bytes) -> str:
    """
    Generates a SHA-256 signature for legal chain-of-custody.
    Concatenates metadata string with raw image bytes.
    """
    hasher = hashlib.sha256()
    hasher.update(metadata_str.encode('utf-8'))
    hasher.update(image_bytes)
    return hasher.hexdigest()

# Usage Example:
# metadata = "2023-10-27T12:00:00Z|ABC-1234|2022 Ford Explorer Black|29.4241|-98.4936|HUB-SN-001"
# signature = calculate_sabre_hash(metadata, raw_crop_bytes)
