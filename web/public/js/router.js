Neutrinoapp.Router.map(function() {
    this.resource('home', { path: '/' });
});

Neutrinoapp.HomeRoute = Ember.Route.extend({
    actions: {
        showModal: function(name, model) {
            this.render(name, {
                into: 'application',
                outlet: 'modal',
                model: model
            });
        },
        removeModal: function() {
            this.disconnectOutlet({
                outlet: 'modal',
                parentView: 'application'
            });
        }
    },
    model: function(){
        return Ember.RSVP.hash({
            "hvacControllers": this.store.find("hvaccontroller"),
            "sensorGroups": this.store.find("sensorgroup"),
            "sensorStats": this.store.find("sensorstat"),
            "sensors" : this.store.find("sensor"),
            "weather" : this.store.find("weather"),
            "statStrings": ["fahrenheit","humidity","voltage"]
        });
    },
    setupController: function(controller, model) {
        controller.set('sensorStats', model.sensorStats);
        controller.set('weather', model.weather);
        controller.set('sensors', model.sensors);
        controller.set('hvacControllers', model.hvacControllers);
        controller.set('sensorGroups', model.sensorGroups);
        controller.set('statStrings', model.statStrings);
    }
});

Neutrinoapp.SensorsRoute = Ember.Route.extend({
    model: function() {
        return this.store.find('sensor');
    }
});

Neutrinoapp.HvaccontrollersRoute = Ember.Route.extend({
    model: function() {
        return this.store.find('hvaccontroller');
    }
});

Neutrinoapp.SensorgroupsRoute = Ember.Route.extend({
    model: function() {
        return this.store.find('sensorgroup');
    }
});

Neutrinoapp.SensorstatsRoute = Ember.Route.extend({
    model: function() {
        return this.store.find('sensorstat');
    }
});
