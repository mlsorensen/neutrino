function componentToHex(c) {
    var hex = c.toString(16);
    return hex.length == 1 ? "0" + hex : hex;
}

function rgbToHex(r, g, b) {
    return "#" + componentToHex(r) + componentToHex(g) + componentToHex(b);
}

function hexToRgb(hex) {
    var result = /^#?([a-f\d]{2})([a-f\d]{2})([a-f\d]{2})$/i.exec(hex);
    return result ? {
        r: parseInt(result[1], 16),
        g: parseInt(result[2], 16),
        b: parseInt(result[3], 16)
    } : null;
}

function chooseColor(startcolor, id) {
    var rgb = hexToRgb(startcolor);
    var multiplier = id * 50;
    return rgbToHex((rgb.r + multiplier) % 255, (rgb.g + multiplier) % 255, (rgb.b + multiplier) % 255);
}

function chooseRed() {
    return "#AA0000";
}
