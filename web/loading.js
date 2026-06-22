/**
 * Ruins of the Ancients — WASM loading screen controller.
 * Updates progress bar during download, hides loading screen when ready,
 * and shows error message if WASM fails to load.
 */

(function() {
    'use strict';

    var progressBarInner = null;
    var progressText     = null;
    var loadingScreen    = null;
    var errorText        = null;
    var canvasContainer  = null;

    /** Cache DOM references once ready. */
    function cacheDom() {
        progressBarInner = document.getElementById('progress-bar-inner');
        progressText     = document.getElementById('progress-text');
        loadingScreen    = document.getElementById('loading-screen');
        errorText        = document.getElementById('error-text');
        canvasContainer  = document.getElementById('canvas-container');
    }

    /**
     * Update the progress bar.
     * @param {number} ratio - Value between 0.0 and 1.0.
     */
    window.updateProgress = function(ratio) {
        if (!progressBarInner) cacheDom();

        var pct = Math.min(Math.max(Math.round(ratio * 100), 0), 100);

        if (progressBarInner) {
            progressBarInner.style.width = pct + '%';
        }
        if (progressText) {
            progressText.textContent = pct + '%';
        }
    };

    /**
     * Called when the WASM runtime is fully initialized and ready.
     * Hides loading screen, reveals canvas, and focuses it for keyboard input.
     */
    window.onWASMReady = function() {
        if (!loadingScreen) cacheDom();

        // Ensure progress shows 100%
        window.updateProgress(1.0);

        // Brief delay so the user sees 100% before transition
        setTimeout(function() {
            if (loadingScreen) {
                loadingScreen.classList.add('hidden');
            }

            // Remove loading screen from DOM after CSS transition
            setTimeout(function() {
                if (loadingScreen && loadingScreen.parentNode) {
                    loadingScreen.parentNode.removeChild(loadingScreen);
                }
            }, 600);

            // Focus canvas for keyboard input
            var canvas = document.getElementById('canvas');
            if (canvas) {
                canvas.focus();
            }
        }, 300);
    };

    /**
     * Show an error message on the loading screen.
     * @param {string} message - Error description.
     */
    window.onWASMError = function(message) {
        if (!errorText) cacheDom();

        if (errorText) {
            errorText.textContent = message;
            errorText.classList.add('visible');
        }
        if (progressText) {
            progressText.textContent = 'Failed';
        }

        // Update subtitle
        var subtitle = document.getElementById('loading-subtitle');
        if (subtitle) {
            subtitle.textContent = 'Something went wrong';
        }
    };

    // ---- Global error handler for WASM load failures ----
    window.addEventListener('error', function(event) {
        // Only handle WASM-related errors during loading
        if (loadingScreen && !loadingScreen.classList.contains('hidden')) {
            var msg = 'Failed to load game.';
            if (event.message) {
                msg += ' ' + event.message;
            }
            window.onWASMError(msg);
        }
    });

    // ---- Handle fetch/network errors for the WASM binary ----
    window.addEventListener('unhandledrejection', function(event) {
        if (loadingScreen && !loadingScreen.classList.contains('hidden')) {
            var msg = 'Network error while loading game.';
            if (event.reason && event.reason.message) {
                msg += ' ' + event.reason.message;
            }
            window.onWASMError(msg);
        }
    });

    // ---- Timeout fallback: if WASM hasn't loaded after 120s, show error ----
    // WASM binary is ~2.5MB; slow connections may need more time.
    var loadTimeout = setTimeout(function() {
        if (loadingScreen && !loadingScreen.classList.contains('hidden')) {
            window.onWASMError('Loading timed out. Please refresh the page.');
        }
    }, 120000);

    // Clear timeout when WASM is ready
    var originalOnReady = window.onWASMReady;
    window.onWASMReady = function() {
        clearTimeout(loadTimeout);
        originalOnReady();
    };

    // Initialize DOM references when ready
    if (document.readyState === 'loading') {
        document.addEventListener('DOMContentLoaded', cacheDom);
    } else {
        cacheDom();
    }
})();
