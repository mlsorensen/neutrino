window.Neutrinoapp = Ember.Application.create();
Neutrinoapp.ApplicationAdapter = DS.RESTAdapter.extend({
  namespace: 'api'
});
