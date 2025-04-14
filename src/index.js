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

function assemble(source) {
    let output_paragraph = document.getElementById('asm-output');
    let simulator_link = document.getElementById('simulator-link');
    let output_box = document.getElementById('output-box');
    let error_paragraph = document.getElementById('error-output');

    const result = Module.ccall(
        'assemble',
        'string',
        ['string'],
        [source]);

    const errors = Module.ccall('get_error_buf', 'string', [], []);

    if (result.length != 0) {
        output_paragraph.innerText = result;
        simulator_link.href = `https://mykbamberg.github.io/cpu-simulator/?ram=${result.replace(/ /g, '')}`;
        output_box.classList.remove('hidden');
    } else {
        output_box.classList.add('hidden');
    }
    error_paragraph.innerHTML = errors;
}

document.addEventListener('DOMContentLoaded', () => {
    /* Configure ace editor element */
    let editor = ace.edit('editor');

    const updateTheme = () => {
        editor.setTheme(window.matchMedia('(prefers-color-scheme: dark)').matches ?
            'ace/theme/tomorrow_night' :
            'ace/theme/tomorrow');
    };
    updateTheme();

    window.matchMedia('(prefers-color-scheme: dark)').addEventListener('change', updateTheme);

    editor.setOptions({
        fontSize: '16pt',
        fontFamily: 'Roboto Mono'
    });

    editor.session.on('change', () => {
        assemble(editor.getValue());
    });

    editor.session.setMode('ace/mode/asm');

    /* Example loading */
    const example_selection = document.getElementById('example-selection');
    const example_load_button = document.getElementById('example-load');

    Object.keys(examples).forEach((name) => {
        const option = document.createElement('option');
        option.value = option.innerText = name;
        example_selection.appendChild(option);
    });

    example_load_button.addEventListener('click', () => {
        editor.setValue(examples[example_selection.value]);
    });

    /* Toggle input mode */
    const vim_mode_button = document.getElementById('toggle-vim');
    const button_inner = vim_mode_button.innerHTML;
    let vim_enabled = false;
    vim_mode_button.addEventListener('click', () => {
        vim_enabled = !vim_enabled;
        editor.setKeyboardHandler(vim_enabled ? 'ace/keyboard/vim' : '');
        vim_mode_button.innerHTML = button_inner.replace('Default', vim_enabled ? 'Vim' : 'Default');
    });

    /* Share button */
    const share_code_button = document.getElementById('share-code');
    share_code_button.addEventListener('click', async () => {
        await navigator.clipboard.writeText(new URL(`?code=${
                btoa(editor.getValue()).replace(/\+/g, '-').replace(/\//g, '_').replace(/=/g, '')
            }`, window.location).href);

        /* Show label */
        const label = document.getElementById('copy-label');
        const editorRect = document.getElementById('editor').getBoundingClientRect();
        label.style.display = 'block';
        label.style.left = editorRect.left + (editorRect.width - label.offsetWidth) / 2 + 'px';
        label.style.top = editorRect.top + (editorRect.height - label.offsetHeight) / 2 + 'px';
        label.style.opacity = 1;
        label.style.transition = 'opacity 1s';

        setTimeout(() => {
            label.style.opacity = 0;
            setTimeout(() => {
                label.style.display = 'none';
                label.style.opacity = 1;
            }, 1000);
        }, 100);
    });

    const code_parameter = new URLSearchParams(window.location.search).get('code');
    if (code_parameter) {
        Module.onRuntimeInitialized = () => {
            editor.setValue(atob(code_parameter.replace(/-/g, '+').replace(/_/g, '/')));
        }
    }

    /* Log toggles */
    document.querySelectorAll('#show-info,#show-warn,#show-error').forEach((element) => {
        element.checked = true;
        element.addEventListener('change', () => {
            const type = element.id.replace('show-', '');
            document.getElementById(`${type}-style`).innerText = element.checked ? '' : `
                .${type} \{
                    display: none;
                \}
            `;
        });
    });
});

const examples = {
    "Addition" : `\
; Addition

    ; get and store user input
    inp
    str $a
    inp
    str $b

    ; load memory stored in the address of label $a
    ; into the ACC register of the processor
    lod $a
    ; add value at label $b to ACC
    add $b
    ; output value of ACC to the user
    out
    end
a:  ; Allocated memory for user input
    0x00
b:
    0x00`,
    "Absolute Difference" : `\
; Absolute difference

    inp
    str $a
    inp
    str $b
    lod $a
    sub $b      ; ACC = a - b
    bnn $output ; if ACC >= 0; then output ACC
    lod $b      ; else ACC = b - a
    sub $a
output:
    out
    end

a:
    0x00
b:
    0x00`,
    "Fibonacci" : `\
; Fibonacci Sequence

; a(n) = a(n-2) + a(n-1)
; a(0) = 0
; a(1) = 1

loop:
    lod $a    ; a(n)
    out
    add $b    ; a(n+1)
    str $tmp  ; a(n+2)
    lod $b
    str $a    ; a(n+1) -> a(n)
    lod $tmp
    str $b    ; a(n+2) -> a(n+1)
    lod $a
    bnn $loop ; break on integer overflow
    end

tmp:
    0x00
a:
    0x00
b:
    0x01`,
    "Multiplication" : `\
; Multiplication by repeated addition
a:  ; reuse code as storage to fit in RAM
    inp
b:
    str $a
    inp
    str $b
loop:
    lod $out
    add $a
    str $out

    ; decrement $b until 0
    lod $b
    sub $one
    biz $end

    str $b
    bch $loop
end:
    lod $out
    out
one:
    ; 0x01 terminates execution as it shares
    ; the \`end' opcode
    0x01
out:
    0x00`,
    "Square Numbers" : `\
; Generate square number sequence
; n^2 = sum_(m=0)^(n-1)2m+1

loop:
    lod $result
    add $odd_num
    bnn $continue
    end ; break on integer overflow
continue:
    out
    str $result
    lod $odd_num
    add $two
    str $odd_num
    bch $loop

two:
    0x02
result:
    0x00
odd_num:
    0x01`,
    "Division" : `\
; Division by repeated subraction

    ; get and store inputs
    ; reuse code as storage to fit in memory
numerator:
    inp
denominator:
    str $numerator
    inp
    str $denominator

    ; repeatedly subtract denominator from numerator until result is negative
    ; loop count equals quotient
loop:
    lod $numerator
    sub $denominator
    str $numerator
    bnn $continue

    ; output and terminate execution
    lod $quotient
    out
one:
    ; 0x01 terminates execution as it shares
    ; the \`end' opcode
    0x01
continue:
    lod $quotient
    add $one
    str $quotient
    bch $loop

quotient:
    0x00`,
};
