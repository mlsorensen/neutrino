///////////////////
// form controls //
///////////////////

// populate form
$(document).on('DOMNodeInserted', function(event) {
    if (event.target.id == 'setup-panel') {
        var interval = setInterval(function() {
            if (config == undefined) {
                return;
            }
            clearInterval(interval);
            // populate location
            $("#location-input").val(config.location.value);

            // populate temp units
            if (config.tempunits.value == "C") {
                $("#tempunit-selector-C").addClass('btn-primary');
            } else if (config.tempunits.value == "F") {
                $("#tempunit-selector-F").addClass('btn-primary');
            }

            //populate graph time
            for (var item in GRAPHTIME) {
                $("#graph-time-list").append('<li><a class="graph-time-selector" href="#">' + GRAPHTIME[item] + '</a></li>');
            }
            if (config.graphtime != undefined) {
                $("#graph-time-button").html(GRAPHTIME[config.graphtime.value]);
            }
        }, 200);
    }
});

//toggle of temp units
$(document.body).on("mouseup",".tempunit-selector", function(event) {
    // set all to default, then change ours to primary
    $(".tempunit-selector").removeClass('btn-primary');
    $(".tempunit-selector").addClass('btn-default');
    $(this).removeClass('btn-default');
    $(this).addClass('btn-primary');
});

// graph time dropdown
var GRAPHTIME = {
    1   : "1 hr",
    3   : "3 hrs",
    6   : "6 hrs",
    12  : "12 hrs",
    24  : "1 day",
    168 : "1 week"
};

$(document.body).on("mouseover",".graph-time-selector", function(event) {
    $(".graph-time-selector").removeClass('active');
    $(this).addClass('active');
});

$(document.body).on("mouseup",".graph-time-selector", function(event) {
    $(".graph-time-selector").removeClass('active');
    $("#graph-time-button").html($(this).html());
});


// save configs from setup page
$(document.body).on("mouseup","#submit-setup-form", function() {
    var success = true;

    var tempunits = 'Fahrenheit';
    if ($(".tempunit-selector.btn-primary").html() == "C") {
        tempunits = 'Celsius';
    }

    var graphtime = 0;
    for (var item in GRAPHTIME) {
        if ( GRAPHTIME[item] == $("#graph-time-button").html() ) {
            graphtime = item;
            break;
        }
    }    
    

    // should tighten these down into a single call with a body containing all of the key/value pairs
    $.ajax({
        type: "POST",
        url: "/api/config/location",
        data: {"value": $("#location-input").val()},
        async: false,
        error: function(data) {
                    $("#submit-setup-form").notify("error on location setting:" + $.parseJSON(data.responseText).text, "error");
                    success=false;
                }
    });

    $.ajax({
        type: "POST",
        url: "/api/config/tempunits",
        data: {"value": tempunits},
        async: false,
        error: function(data) {
                    $("#submit-setup-form").notify("error on temperature units setting:" + $.parseJSON(data.responseText).text, "error");
                    success=false;
                }
    });

    if (graphtime != 0) {
        $.ajax({
            type: "POST",
            url: "/api/config/graphtime",
            data: {"value": graphtime},
            async: false,
            error: function(data) {
                        $("#submit-setup-form").notify("error on location setting:" + $.parseJSON(data.responseText).text, "error");
                        success=false;
                    }
        });
    }

    if (success) {
        getConfig();
        getWeather();
        $("#submit-setup-form").notify("success", "info");
    }
});
