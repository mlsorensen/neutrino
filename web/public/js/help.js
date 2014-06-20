///////////////////
// form controls //
///////////////////

// populate form
$(document).on('DOMNodeInserted', function(event) {
    if (event.target.id == 'setup-form') {
        var interval = setInterval(function() {
            if (config == undefined) {
                return;
            }
            clearInterval(interval);
            // populate location
            $("#location").val(config.location.value);

            // populate temp units
            $('#setup-form :input[value="' + config.tempunits.value + '"]').prop('checked', true);
        }, 200);
    }
});

$(document.body).on("mouseup","#submit-setup-form", function() {
    var success = false;
    // push location element
    $.post("/api/config/location", {"value": $("#location").val()}, function(data) {
        // success
    }).error(function(data) {
        $("#submit-setup-form").notify("error on location setting:" + $.parseJSON(data.responseText).text, "error");
    });
    
    // push temperature preference
    $.post("/api/config/tempunits", {"value": $('input[name=setup-temperature-units]:checked', '#setup-form').val()}, function(data) {
        //success
    }).error(function(data) {
        $("#submit-setup-form").notify("error on temperature preference:" + $.parseJSON(data.responseText).text, "error");
    });

    getConfig();

    $("#submit-setup-form").notify("success", "info");
});
