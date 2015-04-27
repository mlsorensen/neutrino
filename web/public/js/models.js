// models
Neutrinoapp.Weather = DS.Model.extend({
    icon : DS.attr('string'),
    celsius : DS.attr('number'),
    fahrenheit : DS.attr('number'),
    location : DS.attr('string'),
    description : DS.attr('string'),
    humidity : DS.attr('number'),
    didLoad: function() {
        var self = this;
        setInterval(function() {
            if (!self.get('isDeleted')) {
                self.reload();
            }
        }, 300000);
    }
});

Neutrinoapp.Sensor = DS.Model.extend({
    sensor_hub_id : DS.attr('number'),
    sensor_address : DS.attr('number'),
    display_name : DS.attr('string'),
    drives_hvac : DS.attr('boolean'),
    sensorstat_id : DS.belongsTo('sensorstat',{async:true})
});

Neutrinoapp.Sensorgroup = DS.Model.extend({
    display_name: DS.attr('string'),
    members: DS.hasMany('sensor'),
    controller_id: DS.belongsTo('hvaccontroller',{async:true}),
    stats: DS.belongsTo('sensorgroupstat'),
    didLoad: function() {
        var self = this;
        setInterval(function() {
            if (!self.get('isDeleted')) {
                self.reload();
            }
        },30000);
    }
});

Neutrinoapp.Sensorgroupstat = DS.Model.extend({
    sensorgroup_id: DS.belongsTo('sensorgroup'),
    pressure: DS.attr('number'),
    temperature: DS.attr('number'),
    humidity: DS.attr('number')
});

Neutrinoapp.Hvaccontroller = DS.Model.extend({
    fan_mode: DS.attr('string'),
    status: DS.attr('string'),
    display_name: DS.attr('string'),
    enabled: DS.attr('boolean'),
    capabilities: DS.attr('raw'),
    sensorgroup_id: DS.belongsTo('sensorgroup', {async:true}),
    didLoad: function() {
        var self = this;
        setInterval(function() {
            if (!self.get('isDeleted')) {
                self.reload();
            }
        },10000);
    }
});

Neutrinoapp.Sensorstat = DS.Model.extend({
    pascals: DS.attr('raw'),
    humidity: DS.attr('raw'),
    fahrenheit: DS.attr('raw'),
    celsius: DS.attr('raw'), 
    voltage: DS.attr('raw'),
    sensor_id: DS.belongsTo('sensor',{async:true}),

    didLoad: function() {
        var self = this;
        setInterval(function() {
            if (!self.get('isDeleted')) {
                self.reload();
            }
        },30000);
    }
});

// transforms

DS.RawTransform = DS.Transform.extend({
    deserialize: function(value) {
        return value; 
    },
    serialize: function(value) {
        return value;
    }
});

DS.ArrayTransform = DS.Transform.extend({
  deserialize: function(serialized) {
    return (Ember.typeOf(serialized) == "array")
        ? serialized 
        : [];
  },

  serialize: function(deserialized) {
    var type = Ember.typeOf(deserialized);
    if (type == 'array') {
        return deserialized
    } else if (type == 'string') {
        return deserialized.split(',').map(function(item) {
            return jQuery.trim(item);
        });
    } else {
        return [];
    }
  }
});

Neutrinoapp.register("transform:array", DS.ArrayTransform);
Neutrinoapp.register("transform:raw", DS.RawTransform);
