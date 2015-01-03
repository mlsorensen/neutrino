window.Neutrinoapp = Ember.Application.create();
Neutrinoapp.ApplicationAdapter = DS.RESTAdapter.extend({
    namespace: 'api'
});

var inflector = Ember.Inflector.inflector;
inflector.uncountable('weather');
