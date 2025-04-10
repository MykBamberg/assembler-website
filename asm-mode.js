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
