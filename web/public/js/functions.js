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

var opts = {
  lines: 17, // The number of lines to draw
  length: 40, // The length of each line
  width: 19, // The line thickness
  radius: 32, // The radius of the inner circle
  corners: 0.6, // Corner roundness (0..1)
  rotate: 0, // The rotation offset
  direction: 1, // 1: clockwise, -1: counterclockwise
  color: '#176287', // #rgb or #rrggbb or array of colors
  speed: 0.7, // Rounds per second
  trail: 100, // Afterglow percentage
  shadow: false, // Whether to render a shadow
  hwaccel: false, // Whether to use hardware acceleration
  className: 'spinner', // The CSS class to assign to the spinner
  zIndex: 2e9, // The z-index (defaults to 2000000000)
  top: '50%', // Top position relative to parent
  left: '50%' // Left position relative to parent
};
var target = document.getElementById('loader');
var spinner = new Spinner(opts).spin(target);
