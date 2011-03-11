var assert = require('assert');
var Buffer = require('buffer').Buffer;
var zlib = require('../lib/zlib');

exports['test deflate/inflate buffer'] = function(beforeExit) {
    var input = new Buffer('Lorem ipsum dolor sit amet, consectetur adipisicing elit, sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.');
    var compressed = zlib.deflate(input);
    var output = zlib.inflate(compressed);
    assert.deepEqual(input, output);
};
