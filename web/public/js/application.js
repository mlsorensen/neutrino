window.Neutrinoapp = Ember.Application.create();
Neutrinoapp.ApplicationAdapter = DS.RESTAdapter.extend({
    namespace: 'api'
});

Neutrinoapp.MyModalComponent = Ember.Component.extend({
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

Neutrinoapp.ApplicationRoute = Ember.Route.extend({
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
  }
});

var inflector = Ember.Inflector.inflector;
inflector.uncountable('weather');
