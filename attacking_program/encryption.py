from cryptography.hazmat.primitives.ciphers import Cipher, algorithms, modes
from cryptography.hazmat.backends import default_backend
from cryptography.hazmat.primitives import padding
from config import AES_KEY, AES_IV


def aes_encrypt(plaintext):
    # Convertir la chaîne en bytes si nécessaire
    if isinstance(plaintext, str):
        data = plaintext.encode('utf-8')
    else:
        data = plaintext

    # Ajouter un padding PKCS#7
    padder = padding.PKCS7(algorithms.AES.block_size).padder()
    padded_data = padder.update(data) + padder.finalize()

    # Chiffrement CBC
    cipher = Cipher(algorithms.AES(AES_KEY), modes.CBC(AES_IV), backend=default_backend())
    encryptor = cipher.encryptor()
    encrypted = encryptor.update(padded_data) + encryptor.finalize()

    return encrypted


def aes_decrypt(ciphertext):
    # Déchiffrement CBC
    cipher = Cipher(algorithms.AES(AES_KEY), modes.CBC(AES_IV), backend=default_backend())
    decryptor = cipher.decryptor()
    decrypted_padded = decryptor.update(ciphertext) + decryptor.finalize()

    # Supprimer le padding PKCS#7
    unpadder = padding.PKCS7(algorithms.AES.block_size).unpadder()
    unpadded = unpadder.update(decrypted_padded) + unpadder.finalize()

    return unpadded.decode('utf-8', errors='ignore')
