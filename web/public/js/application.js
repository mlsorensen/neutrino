window.Neutrinoapp = Ember.Application.create();
Neutrinoapp.ApplicationAdapter = DS.RESTAdapter.extend({
    namespace: 'api'
});

Neutrinoapp.GenericModalComponent = Ember.Component.extend({
  actions: {
    ok: function() {
      this.$('.modal').modal('hide');
      this.sendAction('ok');
    }
  },
  show: function() {
    this.$('.modal').modal().on('hidden.bs.modal', function() {
      this.sendAction('close');
    }.bind(this));
  }.on('didInsertElement')
});

var inflector = Ember.Inflector.inflector;
inflector.uncountable('weather');
inflector.uncountable('configuration');
