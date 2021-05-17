# unplayer
Simple music player for Sailfish OS

## Building
1. Clone repo:
```sh
git clone https://github.com/equeim/unplayer
cd unplayer
git submodule update --init
```
2. Install Sailfish SDK
3. Rad [this](https://sailfishos.org/wiki/Tutorial_-_Building_packages_-_advanced_techniques) tutorial to learn how to use `sfdk` tool.
4. Build package:
```sh
cd /path/to/sources
sfdk -c no-fix-version -c target=<target name, e.g. SailfishOS-3.3.0.16-armv7hl> build -p -d -j<number of jobs>
```
5. Built RPMs will be in the `RPMS` directory.

## Translations
[![Translation status](https://hosted.weblate.org/widgets/unplayer/-/svg-badge.svg)](https://hosted.weblate.org/engage/unplayer/?utm_source=widget)

Translations are managed on [Hosted Weblate](https://hosted.weblate.org/projects/unplayer/translations).

## Donate
I you like this app, you can support its development via 
[PayPal](https://www.paypal.com/cgi-bin/webscr?cmd=_donations&business=DDQTRHTY5YV2G&item_name=Support%20Unplayer%20development&no_note=1&item_number=1&no_shipping=1&currency_code=EUR) or [Yandex.Money](https://yasobe.ru/na/equeim_unplayer).
