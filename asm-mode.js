/*
Copyright (C) 2025  Mykolas Bamberg

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU Affero General Public License as
published by the Free Software Foundation, either version 3 of the
License, or (at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU Affero General Public License for more details.

You should have received a copy of the GNU Affero General Public License
along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

ace.define('ace/mode/asm', ['require', 'exports', 'module', 'ace/lib/oop', 'ace/mode/text', 'ace/mode/text_highlight_rules'],
    function(require, exports, _) {
        class AsmHighlightRules extends require("ace/mode/text_highlight_rules").TextHighlightRules {
            constructor() {
                super();
                this.$rules = {
                    "start": [
                        {
                            token: 'comment',
                            regex: /;.*$/,
                        }, {
                            token: 'label',
                            regex: /^.*:/,
                        }, {
                            token: 'label_reference',
                            regex: /\$[^ ;]*/,
                        }, {
                            token: 'instruction',
                            regex: /^ *(?:end|add|sub|str|lod|bch|biz|bnn|inp|out)\b/,
                        }, {
                            token: 'number',
                            regex: /\b0x.{1,2}/,
                        },
                    ]
                };
            }
        }

        class Mode extends require("ace/mode/text").Mode {
            constructor() {
                super();
                this.HighlightRules = AsmHighlightRules;
            }
        }

        exports.Mode = Mode;
    });
