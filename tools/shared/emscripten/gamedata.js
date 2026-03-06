// Game data manager for TRX WebGL builds.
//
// Handles uploading, extracting, and persisting user game files in IndexedDB
// so they only need to be provided once.  On return visits the data is loaded
// from IndexedDB directly into the Emscripten virtual filesystem.
//
// Depends on: fflate (UMD, expected as global `fflate`)
//             Emscripten FS  (global `FS`)
//             Emscripten Module (global `Module`)

'use strict';

// ---------------------------------------------------------------------------
// GameDataManager
// ---------------------------------------------------------------------------

var GameDataManager = (function () {
    var DB_VERSION = 1;

    // Game file extensions we care about, grouped by destination directory.
    var LEVEL_EXTS = ['.phd', '.tr2', '.psx', '.tub'];
    var MUSIC_EXTS = ['.flac', '.ogg', '.mp3', '.wav'];
    var SFX_EXTS   = ['.sfx'];
    var FMV_EXTS   = ['.mp4', '.rpl', '.ogv', '.avi', '.fmv'];
    var AUDIO_EXTS = ['.wad'];  // cdaudio.wad for TR3
    var CUT_EXTS   = ['.tr2'];  // cutscene files live in cuts/

    // Remaster-only extensions to skip (textures, models, palettes, etc.).
    var REMASTER_SKIP_EXTS = ['.trg', '.dds', '.trm', '.pdp', '.map', '.tex'];

    // All extensions considered "game data" (for preload dump detection).
    var ALL_GAME_EXTS = LEVEL_EXTS.concat(MUSIC_EXTS, SFX_EXTS, FMV_EXTS, AUDIO_EXTS);

    // Known directory names (case-insensitive) that indicate the game root.
    var ROOT_MARKERS = ['data', 'fmv', 'music', 'audio', 'cuts', 'tracks', 'sfx'];

    // -----------------------------------------------------------------------
    // Constructor
    // -----------------------------------------------------------------------

    function GameDataManager(gameId) {
        this.gameId = gameId;  // "tr1", "tr2", or "tr3"
        this._dbName = 'trx-gamedata-' + gameId;
        this._db = null;
    }

    // -----------------------------------------------------------------------
    // IndexedDB helpers
    // -----------------------------------------------------------------------

    GameDataManager.prototype._openDB = function () {
        var self = this;
        if (self._db) return Promise.resolve(self._db);
        return new Promise(function (resolve, reject) {
            var req = indexedDB.open(self._dbName, DB_VERSION);
            req.onupgradeneeded = function () {
                var db = req.result;
                if (!db.objectStoreNames.contains('files')) {
                    db.createObjectStore('files');
                }
                if (!db.objectStoreNames.contains('meta')) {
                    db.createObjectStore('meta');
                }
            };
            req.onsuccess = function () { self._db = req.result; resolve(self._db); };
            req.onerror = function () { reject(req.error); };
        });
    };

    GameDataManager.prototype._putFile = function (db, path, data) {
        return new Promise(function (resolve, reject) {
            var tx = db.transaction('files', 'readwrite');
            tx.objectStore('files').put(data, path);
            tx.oncomplete = resolve;
            tx.onerror = function () { reject(tx.error); };
        });
    };

    GameDataManager.prototype._putMeta = function (db, info) {
        return new Promise(function (resolve, reject) {
            var tx = db.transaction('meta', 'readwrite');
            tx.objectStore('meta').put(info, 'info');
            tx.oncomplete = resolve;
            tx.onerror = function () { reject(tx.error); };
        });
    };

    GameDataManager.prototype._getMeta = function (db) {
        return new Promise(function (resolve, reject) {
            var tx = db.transaction('meta', 'readonly');
            var req = tx.objectStore('meta').get('info');
            req.onsuccess = function () { resolve(req.result || null); };
            req.onerror = function () { reject(req.error); };
        });
    };

    GameDataManager.prototype._getAllFiles = function (db, onFile) {
        return new Promise(function (resolve, reject) {
            var tx = db.transaction('files', 'readonly');
            var store = tx.objectStore('files');
            var req = store.openCursor();
            req.onsuccess = function () {
                var cursor = req.result;
                if (cursor) {
                    onFile(cursor.key, cursor.value);
                    cursor.continue();
                }
            };
            tx.oncomplete = resolve;
            tx.onerror = function () { reject(tx.error); };
        });
    };

    // -----------------------------------------------------------------------
    // Public API
    // -----------------------------------------------------------------------

    // Check whether game data has been stored in IndexedDB.
    // Also validates that stored paths look sane (no URL-encoded slashes).
    GameDataManager.prototype.hasGameData = function () {
        var self = this;
        return self._openDB().then(function (db) {
            return new Promise(function (resolve, reject) {
                var tx = db.transaction('meta', 'readonly');
                var req = tx.objectStore('meta').get('info');
                req.onsuccess = function () {
                    if (!req.result) { resolve(false); return; }
                    // Spot-check the first stored path for URL-encoded
                    // slashes (%2f) which indicate a previous bad upload.
                    var checkTx = db.transaction('files', 'readonly');
                    var cursor = checkTx.objectStore('files').openCursor();
                    cursor.onsuccess = function () {
                        var c = cursor.result;
                        if (c && c.key.indexOf('%2f') !== -1) {
                            console.warn('[GDM] Detected stale URL-encoded paths in IDB — clearing.');
                            self.clear().then(function () { resolve(false); });
                        } else {
                            resolve(true);
                        }
                    };
                    cursor.onerror = function () { resolve(true); };
                };
                req.onerror = function () { reject(req.error); };
            });
        });
    };

    // Load game data from IndexedDB into the Emscripten FS.
    // Calls `onProgress(loaded, total)` for each file.
    GameDataManager.prototype.loadToFS = function (onProgress) {
        var self = this;
        return self._openDB().then(function (db) {
            return self._getMeta(db).then(function (meta) {
                var total = meta ? meta.fileCount : 0;
                var loaded = 0;
                return self._getAllFiles(db, function (path, data) {
                    _writeToFS(path, new Uint8Array(data));
                    loaded++;
                    if (onProgress) onProgress(loaded, total);
                });
            });
        });
    };

    // Check whether game data was preloaded into FS at build time.
    // Looks for known level file extensions under /data/.
    GameDataManager.prototype.hasPreloadedGameData = function () {
        try {
            var entries = FS.readdir('/data');
            for (var i = 0; i < entries.length; i++) {
                var name = entries[i].toLowerCase();
                for (var j = 0; j < LEVEL_EXTS.length; j++) {
                    if (name.endsWith(LEVEL_EXTS[j])) return true;
                }
                for (var j = 0; j < SFX_EXTS.length; j++) {
                    if (name.endsWith(SFX_EXTS[j])) return true;
                }
            }
        } catch (e) {
            // /data/ might not exist yet
        }
        return false;
    };

    // Dump preloaded FS data into IndexedDB (runs in the background).
    // This lets subsequent visits load from IDB and enables offline use.
    GameDataManager.prototype.dumpPreloadedToIDB = function (onProgress) {
        var self = this;
        var files = [];

        // Recursively walk FS directories for game data files.
        function walk(dir) {
            var entries;
            try { entries = FS.readdir(dir); } catch (e) { return; }
            for (var i = 0; i < entries.length; i++) {
                if (entries[i] === '.' || entries[i] === '..') continue;
                var full = dir + '/' + entries[i];
                var stat;
                try { stat = FS.stat(full); } catch (e) { continue; }
                if (FS.isDir(stat.mode)) {
                    walk(full);
                } else {
                    var lower = entries[i].toLowerCase();
                    var isGameFile = false;
                    for (var j = 0; j < ALL_GAME_EXTS.length; j++) {
                        if (lower.endsWith(ALL_GAME_EXTS[j])) {
                            isGameFile = true;
                            break;
                        }
                    }
                    if (isGameFile) {
                        // Store path without leading slash (e.g. "data/gym.phd")
                        files.push(full.substring(1));
                    }
                }
            }
        }

        walk('/data');
        walk('/music');
        walk('/fmv');
        walk('/cuts');
        walk('/audio');

        if (files.length === 0) return Promise.resolve();

        return self._openDB().then(function (db) {
            var totalBytes = 0;
            var stored = 0;

            function storeNext() {
                if (stored >= files.length) {
                    return self._putMeta(db, {
                        version: 1,
                        fileCount: files.length,
                        totalBytes: totalBytes,
                        uploadDate: new Date().toISOString()
                    });
                }
                var path = files[stored];
                var data;
                try { data = FS.readFile('/' + path); } catch (e) { stored++; return storeNext(); }
                totalBytes += data.byteLength;
                return self._putFile(db, path, data.buffer).then(function () {
                    stored++;
                    if (onProgress) onProgress(stored, files.length);
                    return storeNext();
                });
            }

            return storeNext();
        });
    };

    // Process uploaded files (from file input or drag-and-drop).
    // `items` is a FileList or array of File objects.
    // Calls `onProgress(message, fraction)` with status updates.
    // `onWarning(message)` is called when no FMV cutscenes are found;
    // it must return a Promise that resolves to continue or rejects to cancel.
    GameDataManager.prototype.processUpload = function (items, onProgress, onWarning) {
        var self = this;

        function report(msg, frac) {
            if (onProgress) onProgress(msg, frac);
        }

        // Read all files depending on source type.
        return _readUploadedItems(items, report).then(function (entries) {
            if (entries.length === 0) {
                throw new Error('No game files found in the upload.');
            }

            report('Mapping files...', 0.7);
            var mapped = _mapFiles(entries, self.gameId);
            if (mapped.length === 0) {
                throw new Error('No recognised game files found. Please provide your original Tomb Raider game files.');
            }

            // Fix remastered MAIN.SFX (has an index table prepended).
            for (var i = 0; i < mapped.length; i++) {
                if (_hasExt(_getExt(mapped[i].path), SFX_EXTS)) {
                    mapped[i].data = _stripRemasteredSFXHeader(mapped[i].data);
                }
            }

            // Warn the user if no FMV cutscenes were found.
            var hasFMV = false;
            for (var i = 0; i < mapped.length; i++) {
                if (_isFMV(mapped[i].path)) { hasFMV = true; break; }
            }

            var proceed = hasFMV || !onWarning
                ? Promise.resolve()
                : onWarning(
                    'No FMV videos (.rpl, .ogv, .mp4, .avi) were found. '
                    + 'The game will work, but FMVs will be skipped.'
                );

            return proceed.then(function () {

            report('Saving to browser storage...', 0.75);
            return self._openDB().then(function (db) {
                var totalBytes = 0;
                var stored = 0;

                function storeNext() {
                    if (stored >= mapped.length) {
                        return self._putMeta(db, {
                            version: 1,
                            fileCount: mapped.length,
                            totalBytes: totalBytes,
                            uploadDate: new Date().toISOString()
                        });
                    }
                    var entry = mapped[stored];
                    totalBytes += entry.data.byteLength;
                    return self._putFile(db, entry.path, entry.data.buffer).then(function () {
                        stored++;
                        var frac = 0.75 + 0.2 * (stored / mapped.length);
                        report('Saving ' + entry.path + '...', frac);
                        return storeNext();
                    });
                }

                return storeNext();
            }).then(function () {
                report('Loading into game...', 0.95);
                for (var i = 0; i < mapped.length; i++) {
                    var entry = mapped[i];
                    _writeToFS(entry.path, entry.data);
                }
                report('Done!', 1.0);
            });

            }); // proceed.then
        });
    };

    // Delete all stored game data (for re-upload).
    GameDataManager.prototype.clear = function () {
        var self = this;
        return new Promise(function (resolve, reject) {
            if (self._db) { self._db.close(); self._db = null; }
            var req = indexedDB.deleteDatabase(self._dbName);
            req.onsuccess = resolve;
            req.onerror = function () { reject(req.error); };
        });
    };

    // -----------------------------------------------------------------------
    // File reading (from uploads)
    // -----------------------------------------------------------------------

    function _readUploadedItems(items, report) {
        // Convert to plain array of Files
        var files = [];
        for (var i = 0; i < items.length; i++) {
            files.push(items[i]);
        }

        if (files.length === 0) {
            return Promise.resolve([]);
        }

        // Single file: might be an archive
        if (files.length === 1 && !files[0].webkitRelativePath) {
            var file = files[0];
            var name = file.name.toLowerCase();
            if (name.endsWith('.zip') || name.endsWith('.tar.gz') || name.endsWith('.tgz')) {
                report('Reading archive...', 0.1);
                return _readFileAsArrayBuffer(file).then(function (buf) {
                    if (name.endsWith('.zip')) {
                        report('Extracting ZIP...', 0.2);
                        return _extractZip(new Uint8Array(buf), report);
                    } else {
                        report('Extracting tar.gz...', 0.2);
                        return _extractTarGz(new Uint8Array(buf), report);
                    }
                });
            }
        }

        // Folder upload or multiple files — read each file
        report('Reading files...', 0.1);
        var entries = [];
        var loaded = 0;

        function readNext() {
            if (loaded >= files.length) return Promise.resolve(entries);
            var f = files[loaded];
            var path = f.webkitRelativePath || f.name;
            // Android returns URL-encoded content-URI paths in webkitRelativePath
            // (e.g. "primary%3adownload%2ftr%20data%2f...").  Decode so that
            // %2f becomes "/" and root detection / path splitting works.
            try { path = decodeURIComponent(path); } catch (e) { /* already plain */ }
            return _readFileAsArrayBuffer(f).then(function (buf) {
                entries.push({ path: path, data: new Uint8Array(buf) });
                loaded++;
                report('Reading ' + path + '...', 0.1 + 0.5 * (loaded / files.length));
                return readNext();
            });
        }

        return readNext();
    }

    function _readFileAsArrayBuffer(file) {
        return new Promise(function (resolve, reject) {
            var reader = new FileReader();
            reader.onload = function () { resolve(reader.result); };
            reader.onerror = function () { reject(reader.error); };
            reader.readAsArrayBuffer(file);
        });
    }

    // -----------------------------------------------------------------------
    // Archive extraction
    // -----------------------------------------------------------------------

    function _extractZip(data, report) {
        var entries = [];
        var decompressed = fflate.unzipSync(data);
        var keys = Object.keys(decompressed);
        for (var i = 0; i < keys.length; i++) {
            var path = keys[i];
            // Skip directories (they end with /)
            if (path.endsWith('/')) continue;
            entries.push({ path: path, data: decompressed[path] });
            if (report && i % 50 === 0) {
                report('Extracting ' + path + '...', 0.2 + 0.5 * (i / keys.length));
            }
        }
        return entries;
    }

    function _extractTarGz(data, report) {
        // Decompress gzip first
        if (report) report('Decompressing gzip...', 0.2);
        var tarData = fflate.gunzipSync(data);

        // Parse tar
        if (report) report('Parsing tar archive...', 0.4);
        return _parseTar(tarData, report);
    }

    // Minimal tar parser (POSIX ustar / GNU tar).
    function _parseTar(data, report) {
        var entries = [];
        var offset = 0;

        while (offset + 512 <= data.length) {
            // Check for end-of-archive (two zero blocks)
            var allZero = true;
            for (var i = 0; i < 512; i++) {
                if (data[offset + i] !== 0) { allZero = false; break; }
            }
            if (allZero) break;

            // Parse header
            var name = _tarString(data, offset, 100);
            var size = _tarOctal(data, offset + 124, 12);
            var typeFlag = data[offset + 156];
            var prefix = _tarString(data, offset + 345, 155);

            if (prefix) name = prefix + '/' + name;

            offset += 512; // skip header

            // Type 0 or ASCII '0' = regular file
            if ((typeFlag === 0 || typeFlag === 48) && size > 0) {
                var fileData = data.slice(offset, offset + size);
                entries.push({ path: name, data: fileData });
            }

            // Advance past file data (rounded up to 512-byte blocks)
            offset += Math.ceil(size / 512) * 512;

            if (report && entries.length % 50 === 0) {
                report('Extracting ' + name + '...', 0.4 + 0.3 * (offset / data.length));
            }
        }

        return entries;
    }

    function _tarString(data, offset, len) {
        var s = '';
        for (var i = 0; i < len; i++) {
            if (data[offset + i] === 0) break;
            s += String.fromCharCode(data[offset + i]);
        }
        return s;
    }

    function _tarOctal(data, offset, len) {
        var s = _tarString(data, offset, len).trim();
        return parseInt(s, 8) || 0;
    }

    // -----------------------------------------------------------------------
    // File mapping / normalisation
    // -----------------------------------------------------------------------

    // Map raw extracted entries to VFS paths the engine expects.
    function _mapFiles(entries, gameId) {
        // 1. Find the root by looking for known directory patterns.
        var root = _detectRoot(entries);

        // 2. Strip root prefix, normalise case, and classify each file.
        var mapped = [];
        for (var i = 0; i < entries.length; i++) {
            var rawPath = entries[i].path;
            var data = entries[i].data;

            // Strip detected root prefix
            var relPath = rawPath;
            if (root && relPath.toLowerCase().indexOf(root.toLowerCase()) === 0) {
                relPath = relPath.substring(root.length);
            }

            // Normalise separators and case
            relPath = relPath.replace(/\\/g, '/').replace(/^\/+/, '');
            var lowerPath = relPath.toLowerCase();

            // Skip empty or irrelevant files
            if (!relPath || data.byteLength === 0) continue;

            // Classify and map to VFS path
            var vfsPath = _classifyFile(lowerPath, relPath, gameId);
            if (vfsPath) {
                mapped.push({ path: vfsPath, data: data });
            }
        }

        return mapped;
    }

    // Detect the root directory prefix by looking for common game directory
    // names (data/, fmv/, music/, etc.) in the entry paths.
    function _detectRoot(entries) {
        // Count occurrences of potential roots
        var candidates = {};
        for (var i = 0; i < entries.length; i++) {
            var path = entries[i].path.replace(/\\/g, '/');
            var parts = path.toLowerCase().split('/');
            for (var j = 0; j < parts.length - 1; j++) {
                for (var k = 0; k < ROOT_MARKERS.length; k++) {
                    if (parts[j] === ROOT_MARKERS[k]) {
                        // The root is everything before this directory
                        var root = path.split('/').slice(0, j).join('/');
                        if (root) root += '/';
                        candidates[root] = (candidates[root] || 0) + 1;
                    }
                }
            }
        }

        // Pick the most common root (or empty string if files are at top level)
        var bestRoot = '';
        var bestCount = 0;
        var keys = Object.keys(candidates);
        for (var i = 0; i < keys.length; i++) {
            if (candidates[keys[i]] > bestCount) {
                bestCount = candidates[keys[i]];
                bestRoot = keys[i];
            }
        }

        return bestRoot;
    }

    // Classify a file by its path and extension, returning the VFS path the
    // engine expects, or null if the file should be skipped.
    function _classifyFile(lowerPath, originalRelPath, gameId) {
        var ext = _getExt(lowerPath);
        var lowerBase = lowerPath.split('/').pop();

        // --- Skip remaster-only files (.trg, .dds, .trm, .pdp, .map, .tex) ---
        if (_hasExt(ext, REMASTER_SKIP_EXTS)) {
            return null;
        }

        // --- Level files (.phd for TR1, .tr2 for TR2/TR3) ---
        if (_hasExt(ext, LEVEL_EXTS)) {
            // TR3 cutscene files live in cuts/, identified by being in a
            // "cuts" directory or starting with "cut" prefix.
            var inCutsDir = lowerPath.indexOf('cuts/') === 0;
            if (inCutsDir) {
                return 'cuts/' + lowerBase;
            }
            return 'data/' + lowerBase;
        }

        // --- Sound effects ---
        // Remastered stores SFX under sfx/ (e.g. sfx/main.sfx) instead
        // of data/.  Both layouts map to data/.
        if (_hasExt(ext, SFX_EXTS)) {
            return 'data/' + lowerBase;
        }

        // --- Music tracks ---
        // Remastered stores music under tracks/ (e.g. tracks/2.ogg)
        // instead of music/.  Both layouts map to music/.
        if (_hasExt(ext, MUSIC_EXTS)) {
            return 'music/' + lowerBase;
        }

        // --- FMV cutscenes (.rpl, .ogv, .mp4, .avi — decoded by FFmpeg) ---
        if (_hasExt(ext, FMV_EXTS)) {
            return 'fmv/' + lowerBase;
        }

        // --- Audio WAD (TR3) ---
        if (_hasExt(ext, AUDIO_EXTS)) {
            return 'audio/' + lowerBase;
        }

        return null;  // skip unknown files
    }

    function _getExt(path) {
        var dot = path.lastIndexOf('.');
        return dot >= 0 ? path.substring(dot) : '';
    }

    function _hasExt(ext, list) {
        for (var i = 0; i < list.length; i++) {
            if (ext === list[i]) return true;
        }
        return false;
    }

    function _isFMV(path) {
        return _hasExt(_getExt(path.toLowerCase()), FMV_EXTS);
    }

    // The Remastered release prepends an index table to MAIN.SFX.  TRX
    // expects the file to start with concatenated RIFF/WAVE chunks, so
    // strip everything before the first RIFF signature.
    function _stripRemasteredSFXHeader(data) {
        // Already starts with RIFF — nothing to do.
        if (data.length >= 4
            && data[0] === 0x52 && data[1] === 0x49
            && data[2] === 0x46 && data[3] === 0x46) {
            return data;
        }
        // Scan for the first RIFF signature (aligned to 2-byte boundary).
        for (var i = 2; i <= data.length - 4; i += 2) {
            if (data[i]     === 0x52 && data[i + 1] === 0x49
                && data[i + 2] === 0x46 && data[i + 3] === 0x46) {
                console.log('[GDM] Stripping ' + i + '-byte remastered SFX header');
                return data.slice(i);
            }
        }
        // No RIFF found — return as-is and let the engine report the error.
        return data;
    }

    // -----------------------------------------------------------------------
    // Emscripten FS helpers
    // -----------------------------------------------------------------------

    function _writeToFS(path, data) {
        // Ensure parent directories exist
        var parts = path.split('/');
        var dir = '';
        for (var i = 0; i < parts.length - 1; i++) {
            dir += (dir ? '/' : '') + parts[i];
            try { FS.mkdir('/' + dir); } catch (e) { /* already exists */ }
        }
        FS.writeFile('/' + path, data);
    }

    return GameDataManager;
})();
