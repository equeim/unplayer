# unplayer
Simple music player for Sailfish OS

## Building
1. Clone repo:
```sh
git clone https://github.com/equeim/unplayer
cd unplayer
git submodule init
git submodule update
```
2. Start MerSDK virtual machine (if you installed SDK using official installer).
3. SSH into SDK (or chroot if you have set up SDK manually), see [here](https://sailfishos.org/wiki/Tutorial_-_Building_packages_manually) for additional information.
4. Build package:
```sh
cd /path/to/sources
mb2 -X -t <target name, e.g. SailfishOS-3.0.2.8-armv7hl> build -p -d -j<number of jobs>
```
5. Built RPMs will be in the `RPMS` directory.

## Translations
[![Translation status](https://hosted.weblate.org/widgets/unplayer/-/svg-badge.svg)](https://hosted.weblate.org/engage/unplayer/?utm_source=widget)

Translations are managed on [Hosted Weblate](https://hosted.weblate.org/projects/unplayer/translations).

## Donate
I you like this app, you can support its development via 
[PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=DDQTRHTY5YV2G&item_name=Support%20Unplayer%20development&no_note=1&item_number=1&no_shipping=1&currency_code=EUR) or [Yandex.Money](https://yasobe.ru/na/equeim_unplayer).
