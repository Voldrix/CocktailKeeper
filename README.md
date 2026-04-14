# Cocktail Keeper
### Minimalist Personal Drink Recipes
Source code for [cocktailkeeper.cc](https://cocktailkeeper.cc/)

This code is for a public multi-user website, not a personal single user application.

Example profile with sample data [cocktailkeeper.cc/voldrix](https://cocktailkeeper.cc/voldrix)

## Features
- Minimalistic, simple, fast, uncluttered UI
- No superfluous features. Just your list of recipes
- Clone recipes from other users
- JSON-LD for contextual link sharing and SEO
- Optimistic and asynchronous image upload
- Image scaling with seam carving for aspect ratio changes

### User Profile URL Spam
Every user gets a vanity user profile URL (/user).\
index.html will be served for any unknown wildcard address.\
This will respond to every bot request with the index, which produces too much spam.\
The solution in nginx.conf is to deny any request with a "." (dot) with exceptions for the known required files.\
Dots are not allowed in usernames or drink slugs.\
Username URLs are case insensitive.

### Setup
- mkdir images imagesTmp
- create database and dbUser
- `api.php` add db creds, auth token prefix, and passwd salt
- `prereqs.sh` installs image libraries for scaler
- `scaler/main.c` macros ICON_DIR_IN, ICON_DIR_OUT, ICON_DIR_OUT_LEN, IMG_OWNER_USER_ID
- crontab for tmp image cleanup (from unsaved image changes)
- nginx.conf required for profile urls and api location

### INFO
image scaler log: /var/log/ckeeper.scaler.log\
tmp image cleanu log: ./imgCleanup.log\
supported image types: jpg, png, bmp, webp

images are uploaded to ./imagesTmp until edit is saved\
saved image changes are moved to ./images\
image scaler watches ./images and saves thumbails to ./img/user

### License
[MIT License](LICENSE.txt)
