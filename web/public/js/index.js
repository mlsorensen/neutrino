$(document).ready(function(){
    ////////////////////
    // main nav links //
    ////////////////////

    $(".nav > .dropdown > ul > li > a").on("click", function(event) {
        $(".nav > .dropdown").removeClass('active');
        $( this ).parent().parent().parent().addClass('active');
        $.ajax({
            url: event.target.id.replace("-","/") + ".html.part",
            success: function(data) {
                $("#content-box").html(data);
            }
        });
    });
});
