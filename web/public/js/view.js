Neutrinoapp.HvaccontrollerenabledView = Ember.Checkbox.extend({
    attributeBindings: ['data-toggle'],
    'data-toggle': 'toggle',
    failed: false,
    save: function() {
        this.ctrlr.get('store').find('hvaccontroller',this.ctrlr.get('id')).then(function(thing) {
            if (!this.failed) {
                thing.save().catch(function() {
                    alert("failed to save controller enabled state to " + thing.get('enabled'));
                    if (thing.get('enabled')) {
                        thing.set('enabled',false);
                    } else {
                        thing.set('enabled',true);
                    }
                    this.failed = true;
                });
            } else {
                this.failed = false;
            }
        });
    }.observes('checked')
});

Neutrinoapp.FlotView = Ember.View.extend({
    template:Ember.Handlebars.compile("<div class='chart' ></div>"),

    init: function(){

    },

    chartUpdated: function() {
        this.didInsertElement();
    }.observes('sensorstats.@each.voltage'),

    didInsertElement: function(){
        var statstring = this.stat;
        var members = this.members;
        var data = [];

        members.forEach(function (member) {
            var colored = chooseColor("00AA00", member.get('id'));
            data.push({
                data: member.get('sensorstat_id').get(statstring),
                label: member.get('display_name'),
                color: colored,
                lines: {show:true},
            });
        });

        this.drawChart(data);
    },
    drawChart: function(data){
        var options = {
            axisLabels: {show:true},
            series: {shadowSize:5, lines:{show:true},},
            xaxes: [{mode:"time", timezone: "browser", twelveHourClock: true}],
            yaxes: [{alignTicksWithAxis:1, position:"right", axisLabel:'label', autoscaleMargin:.3 }],
            legend: {position:'sw'}
        };

        $.plot(this.$('.chart'), data, options);
    }
});
