var model;

$(document).ready(function(){
    ////////////////////
    // main nav links //
    ////////////////////

    $(".nav > .dropdown").on("click", function(event) {
        $(".nav > .dropdown").removeClass('active');
        $( this ).addClass('active');
    });

    $(".nav > .dropdown > ul > li > a").on("click", function(event) {
        $.ajax({
            url: event.target.id.replace("-","/") + ".html.part",
            success: function(data) {
                $("#content-box").html(data);
            }
        });
    });
    
    var AppVM = function () {
        this.locale    = ko.observable();
        this.tempunits = ko.observable();
    }

    // Activates knockout.js
    ko.applyBindings(model = new AppVM());

    getLocation();
    getTempUnits();
});
