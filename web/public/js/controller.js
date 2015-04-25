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
        },
        updateSensorNames: function() {
            this.store.find('sensor').then(function(sensors) {
                sensors.forEach(function(sensor) {
                    if (sensor.get("isDirty")) {
                        sensor.save();
                    }
                });
            });
        },
        addSensorGroup: function() {
            var store = this.store;
            store.createRecord('sensorgroup', {
                display_name:this.get("groupname")
            }).save().then(function(result) {
                result.reload().then(function() {result.deleteRecord()});
            });
            this.set("groupname","");
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

Neutrinoapp.SensorgroupItemController = Ember.ObjectController.extend({
    selected: function() {return this.get('parentController.model.members').contains(this.get('content'))}.property(),      
    label: function() {return this.get('display_name')}.property(),
    selectedChanged: function() {
        var sensor = this.get('content');
        var groupmembers = this.get('parentController.model.members');
        if (this.get('selected')) {                                    
            groupmembers.pushObject(sensor);            
        } else {                                    
            groupmembers.removeObject(sensor);                                                    
        }        
        this.get('parentController.model').save().catch(function() {
            alert("failed to save sensor group");
        });
    }.observes('selected')
});

function sanityCheckSetpoint(cap, controller, newsetpoint) {
    console.log(cap);
    console.log(controller);
    console.log(newsetpoint);
    var mindiff = 4;
    var minheat = 50;
    var maxcool = 90;
    var maxhumidify = 50;
    var coolpoint;
    var heatpoint;
    var allcaps = controller.get('capabilities');
    for (i=0; i < allcaps.length; i++) {
        console.log(allcaps[i]);
        if (allcaps[i].capability == 'cool') {
            coolpoint = parseInt(allcaps[i].setpoint);
        } else if (allcaps[i].capability == 'heat') {
            heatpoint = parseInt(allcaps[i].setpoint);
        }
    }
    if (cap.capability == 'heat') {
        if (newsetpoint < minheat) {
            alert('setpoint too low');
            return false;
        }
        if (coolpoint != undefined && newsetpoint > (coolpoint - mindiff)) {
            alert(newsetpoint + ' too close to cool point' + coolpoint + '!');
            return false;
        }
    } else if (cap.capability == 'cool') {
        if (newsetpoint > maxcool) {
            alert('setpoint too high');
            return false;
        }
        if (heatpoint != undefined && coolpoint != undefined && newsetpoint < (heatpoint + mindiff)) {
            alert(newsetpoint + ' too close to heat point ' + heatpoint + '!');
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

Neutrinoapp.SensorgroupSensorController = Ember.Controller.extend({
    actions: {
        logout: function() {
            alert('logout');
        }
    }
});
