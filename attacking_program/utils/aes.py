from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding
import config as cfg

# ------------ AES-128 Encryption/Decryption with PKCS#7 padding ------------ #

def aes_encrypt(plaintext):
    # Convert string to bytes if needed
    if isinstance(plaintext, str):
        data = plaintext.encode('utf-8')
    else:
        data = plaintext
    
    # Apply PKCS#7 padding
    padder = padding.PKCS7(algorithms.AES.block_size).padder()
    padded_data = padder.update(data) + padder.finalize()
    
    # Encrypt the padded data
    cipher = Cipher(algorithms.AES(cfg.AES_KEY), modes.CBC(cfg.AES_IV), backend=default_backend())
    encryptor = cipher.encryptor()
    encrypted = encryptor.update(padded_data) + encryptor.finalize()
    return encrypted


def aes_decrypt(ciphertext):
    # Decrypt the data
    cipher = Cipher(algorithms.AES(cfg.AES_KEY), modes.CBC(cfg.AES_IV), backend=default_backend())
    decryptor = cipher.decryptor()
    decrypted_padded = decryptor.update(ciphertext) + decryptor.finalize()
    
    # Remove PKCS#7 padding
    unpadder = padding.PKCS7(algorithms.AES.block_size).unpadder()
    unpadded = unpadder.update(decrypted_padded) + unpadder.finalize()
    
    # Convert to string
    result = unpadded.decode('utf-8', errors='ignore')
    
    return result