Module.onRuntimeInitialized = function() {
    let output_paragraph = document.getElementById('output');

    const result = Module.ccall(
        'assemble',
        'string',
        ['string'],
        [`
        bch $start
        value:
            0x2a
        start:
            lod $value
            out
            end`]);

    output_paragraph.innerText = result;
}
