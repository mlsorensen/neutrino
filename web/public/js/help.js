///////////////////
// form controls //
///////////////////

// populate form
$(document).on('DOMNodeInserted', function(event) {
    if (event.target.id == 'setup-form') {
        // populate location
        $("#location").val(model.locale());

        // populate temp units
        $('#setup-form :input[value="' + model.tempunits() + '"]').prop('checked', true);
    }
});

$(document.body).on("mouseup","#submit-setup-form", function() {
    var success = false;
    // push location element
    $.post("/api/config/location", {"value": $("#location").val()}, function(data) {
        model.locale($("#location").val());
    }).error(function(data) {
        $("#submit-setup-form").notify("error on location setting:" + $.parseJSON(data.responseText).text, "error");
    });
    
    // push temperature preference
    $.post("/api/config/tempunits", {"value": $('input[name=setup-temperature-units]:checked', '#setup-form').val()}, function(data) {
        model.tempunits($('input[name=setup-temperature-units]:checked', '#setup-form').val());
    }).error(function(data) {
        $("#submit-setup-form").notify("error on temperature preference:" + $.parseJSON(data.responseText).text, "error");
    });

    $("#submit-setup-form").notify("success", "info");
});
