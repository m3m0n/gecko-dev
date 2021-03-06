/* Any copyright is dedicated to the Public Domain.
 * http://creativecommons.org/publicdomain/zero/1.0/ */

/* eslint-env node */

module.exports = function(config) {
  "use strict";

  var baseConfig = require("./karma.conf.base.js")(config);

  // List of files / patterns to load in the browser.
  baseConfig.files = baseConfig.files.concat([
    "standalone/content/vendor/l10n-gaia-02ca67948fe8.js",
    "content/shared/vendor/lodash-3.9.3.js",
    "content/shared/vendor/backbone-1.2.1.js",
    "content/shared/vendor/react-0.13.3.js",
    "content/shared/vendor/classnames-2.2.0.js",
    "content/shared/vendor/sdk.js",
    "test/shared/vendor/*.js",
    "test/shared/loop_mocha_utils.js",
    "test/karma/head.js", // Add test fixture container
    "content/shared/js/loopapi-client.js",
    "content/shared/js/utils.js",
    "content/shared/js/store.js",
    "content/shared/js/models.js",
    "content/shared/js/mixins.js",
    "content/shared/js/crypto.js",
    "content/shared/js/validate.js",
    "content/shared/js/actions.js",
    "content/shared/js/dispatcher.js",
    "content/shared/js/otSdkDriver.js",
    "content/shared/js/activeRoomStore.js",
    "content/shared/js/views.js",
    "content/shared/js/textChatStore.js",
    "content/shared/js/textChatView.js",
    "content/shared/js/urlRegExps.js",
    "content/shared/js/linkifiedTextView.js",
    "standalone/content/js/standaloneAppStore.js",
    "standalone/content/js/standaloneMozLoop.js",
    "standalone/content/js/standaloneRoomViews.js",
    "standalone/content/js/standaloneMetricsStore.js",
    "standalone/content/js/webapp.js",
    "test/shared/*.js",
    "test/standalone/*.js"
  ]);

  // Preprocess matching files before serving them to the browser.
  // Available preprocessors: https://npmjs.org/browse/keyword/karma-preprocessor .
  baseConfig.preprocessors = {
    "content/shared/js/*.js": ["coverage"],
    "standalone/content/js/*.js": ["coverage"]
  };

  baseConfig.coverageReporter.dir = "test/coverage/shared_standalone";

  config.set(baseConfig);
};
