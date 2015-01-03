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
    if (cap.capability == 'heat') {
        if (newsetpoint < 50) {
            alert('setpoint too low');
            return false;
        }
        var coolpoint = parseInt($("#setpoint-" + controller.get('id') + "-cool").html());
        if (coolpoint != undefined && newsetpoint > (coolpoint - 3)) {
            alert('too close to cool point!');
            return false;
        }
    } else if (cap.capability == 'cool') {
        if (newsetpoint > 90) {
            alert('setpoint too high');
            return false;
        }
        var heatpoint = parseInt($("#setpoint-" + controller.get('id') + "-heat").html());
        if (heatpoint != undefined && newsetpoint < (heatpoint + 3)) {
            alert('too close to heat point!');
            return false;
        }
    } else if (cap.capability == 'humidify') {
        if (newsetpoint > 50) {
            alert('setpoint too high');
            return false;
        }
    }
    return true;
}
