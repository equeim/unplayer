# Change Log

## [1.0.1] - 2017-04-02
### Fixed
- Detection of cover images
- Crash when removing deleted cover images from database

## [1.0.0] - 2017-04-01
### Added
- Opening files from command line or file manager (you will need to set Unplayer as default by using xdg-mime command)
- "Loop one track" mode
- Ability so set default directory for Directories page
- "Clear Queue" menu item in pull down menu of Now Playing page and Queue page

### Changed
- Unplayer now doesn't use Tracker. It scans filesystem by itself, extracts tags from music files and stores them in its own database. Note that it doesn't scan whole filesystem, you should add directories yourself (it is ~/Music and SD card by default).
- QMediaPlayer is now used for audio playback (it still uses GStreamer as backend)
- Artists, Albums, Tracks and Genres pages are moved to separate Library page. In settings you can enable opening Library page on startup
- Improvement of About page. License is now shown in HTML format
- Improved support of playlists (still only PLS playlists are supported)

### Fixed
- UI bugs
