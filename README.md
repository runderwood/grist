## Dependencies

The gristmgr program requires libgeos, libtokyocabinet, and libmozjs185.

## Usage

The gristmgr tool provides management facilities for grist databases. Usage is as follows:

    gristmgr v0.01
    usage: gristmgr cmd [options...] path [params...]
        init path
        stat path
        list [-pv] path
        put path [key] wkt-geom json-object
        get path [-wb|-js|-gj|-wt] key
        del path key
        eval path key script
        map path script
        mkview path view [-lu|-js] script
        rmview path view
        vget path view key
        version
        help

### Creating a Database

    >gristmgr init mydatabase.grist

### Database Info

    >gristmgr stat mydatabase.grist

### List Keys

    >gristmgr list mydatabase.grist

### Add a Feature

    >gristmgr put mydatabase.grist myfeature 'POINT (-71.0617 42.3578)' '{"name": "Boston", "nicknames": ["Beantown","St. Botolph\'s Town"]}'

### Get a Feature

    >gristmgr get mydatabase.grist myfeature

### Delete a Feature

    >gristmgr del mydatabase.grist myfeature

### Evaluate a Javascript Function Against a Feature

    >gristmgr eval mydatabase.grist myfeature myjavascript.js

### Map a Javascript Function Against All Features

    >gristmgr map mydatabase.grist myjavascript.js

## Implementation Details

Grist files consist of features and metadata. A feature is a geometry (libgeos) and a "document" (json-serialized object).
Database files are Tokyo Cabinet key-value stores. Javascript functions may be evaluated against individual keys
or against all features in the database:

    >gristmgr eval mygristdb.grist mykey mymapfunction.js
    >gristmgr map mygristdb mymapfunction.js

## Coming Soon
- View generation
- Multiple output formats for map and eval functions(including geojson)
- RESTful server (gristd)


