# Changelog

## [Unreleased]
### Added
- Added option to stop after end of track instead of playing next track in queue
- Library scanning is now cancellable
- Added indication of library scanning progress
- Added command line option to update library without launching GUI

### Changed
- Library scanning and removing of files doesn't block user interaction anymore
- Improved performance and reduced memory usage of library scanning
- Embedded media art files are now always saved to the cache directory regardless of "Prefer directory media art" setting.
Now there is no need rescan library after changing this setting.
- Command line options that do not launch GUI now doesn't require OS graphical session

### Fixed
- Fixed not starting playback the first time if "Restore player state on startup" is enabled and saved queue is empty
- Unplayer now doesn't restore track position if current track wasn't added to queue when restoring player state

## [1.4.1] - 2018-10-16
### Changed
- Updated Spanish translation

### Fixed
- Always return true for MPRIS CanControl property (fixes lockscreen controls)
- Deleted media art files are now removed from database when updating library

## [1.4.0] - 2018-10-15
### Added
- Tracks inside album are now sorted by disc number
- Added support of opening network streams (e.g. podcasts, IP radio)
- Added support of nested playlists

### Changed
- MPRIS interface now expose track's length, current position, repeat and shuffle mode
- Improved performance of library scanning
- Improved performance of adding tracks to queue
- Improved playlists parsing
- CMake build system
- qtmpris and taglib are now linked statically

### Fixed
- Setting of MPRIS trackId property
- Creation of playlists when playlists directory does not exist
- Adding tracks to playlist from queue
- Fixed opening playlists from command line
- Yandex.Money donate link
- Crash when restoring player state

## [1.3.0] - 2018-05-05
### Added
- Donation links via PayPal and Yandex.Money
- Show remaining time on Now Playing page
- Option to show video files in directories view (only audio streams are played, and they will not be showed in library)
- Ability to remove files from database and filesystem
- Blacklist directories (and their subdirectories) from library

### Fixed
- Shuffle "random" track being the same on every app launch
- Multi-selection not resetting in some cases
- MPRIS DBus object doesn't set trackId property

## [1.2.4] - 2017-04-20
### Changed
- Spanish translation fixes

## [1.2.3] - 2017-04-19
### Changed
- Updated Spanish translation

### Fixed
- Artist/album labels on playlist's page

## [1.2.2] - 2017-04-14
### Added
- Added support of files with .aac extension. Note that only MP4 files with this extension are fully supported. Raw AAC streams can be played, but some information will not be shown.

### Changed
- Improved detection of MIME types.

## [1.2.1] - 2017-04-13
### Changed
- Directories that contain .nomedia file are ignored when scanning filesystem

### Fixed
- Extraction of cover art from APE tags (affects APE and some MP3 files). You will need to reset library for changes to take into effect, since Unplayer does not extract tags from files that were already scanned.

## [1.2.0] - 2017-04-13
### Changed
- Library is now automatically updated after changing "Prefer cover art..." option
- Directories view and file file picker dialog now restore list position when returning to parent directory

### Added
- Restoring player state on startup
- Albums list shows album's year
- Track information page now shows track's year
- Changing artists sort order
- Changing albums sort order and criterias
- Changing tracks sort order and criterias
- Changing genres sort order

## [1.1.1] - 2017-04-11
### Changed
- Updated Dutch translation

### Fixed
- Opening artist's tracks page from its context menu
- Search on album page and all tracks page
- Opening M3U playlists from file manager

## [1.1.0] - 2017-04-09
### Added
- Support of tracks with multiple artists/albums/genres
- Support of extracting cover art that is embedded in music files
- M3U playlists support

### Changed
- Changed behaviour of Directories view with playlists:
    - When clicking on music file, all music files in the directory are added to queue, playlists are ignored
    - When clicking on playlist, only this playlist is added to queue, other playlists and music files in the directory are ignored

### Fixed
- Playlists with relative paths

## [1.0.1] - 2017-04-02
### Changed
- Updated Dutch translation

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
