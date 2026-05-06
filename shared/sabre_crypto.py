import hashlib

def generate_sabre_hash(plate: str, timestamp: str, lat: float, lon: float, image_bytes: bytes) -> str:
    """
    Concatenates metadata and image bytes for a unique SHA-256 signature.
    Format: plate|timestamp|lat|lon|image_data
    """
    hasher = hashlib.sha256()
    metadata = f"{plate}|{timestamp}|{lat:.6f}|{lon:.6f}|"
    hasher.update(metadata.encode('utf-8'))
    hasher.update(image_bytes)
    return hasher.hexdigest()
