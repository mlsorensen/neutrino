Neutrinoapp.HomeController = Ember.Controller.extend({
    actions: {
        changeSetpoint: function(cap, controller, change) {
            var current = parseInt(cap.setpoint);
            var newsetpoint = current + change;
            if (sanityCheckSetpoint(cap, controller, newsetpoint)) {
                Ember.set(cap, 'setpoint', newsetpoint);
                controller.get('store').find('hvaccontroller',controller.get('id')).then(function(thing) {
                    thing.save().catch(function() {
                        alert("failed to save new setpoint");
                        Ember.set(cap, 'setpoint', current);
                    });
                });
            }
        }
    },
    datasets: function() {
        return this.store.find('sensorstat');
    }
});

Neutrinoapp.ChartsController = Ember.ArrayController.extend({
  datasets: function() {
    return this.map(function(chart) {
      return chart.get('dataset');
    });
  }.property('@each.dataset')
});

function sanityCheckSetpoint(cap, controller, newsetpoint) {
    var mindiff = 4;
    var minheat = 50;
    var maxcool = 90;
    var maxhumidify = 50;
    if (cap.capability == 'heat') {
        if (newsetpoint < minheat) {
            alert('setpoint too low');
            return false;
        }
        var coolpoint = parseInt($("#setpoint-" + controller.get('id') + "-cool").html());
        if (coolpoint != undefined && newsetpoint > (coolpoint - mindiff)) {
            alert('too close to cool point!');
            return false;
        }
    } else if (cap.capability == 'cool') {
        if (newsetpoint > maxcool) {
            alert('setpoint too high');
            return false;
        }
        var heatpoint = parseInt($("#setpoint-" + controller.get('id') + "-heat").html());
        if (heatpoint != undefined && newsetpoint < (heatpoint + mindiff)) {
            alert('too close to heat point!');
            return false;
        }
    } else if (cap.capability == 'humidify') {
        if (newsetpoint > maxhumidify) {
            alert('setpoint too high');
            return false;
        }
    }
    return true;
}
