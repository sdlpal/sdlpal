openssl x509 -req -days 3650 -in sdlpal.csr -out sdlpal.crt -signkey sdlpal.key -extfile sdlpal.req.cfg -extensions extensions
openssl pkcs12 -in sdlpal.crt -inkey sdlpal.key -out ..\winrt\SDLPal.UWP\SDLPal_TemporaryKey.pfx -export
