local confdir = 'conf/'
if config.install then
    confdir = config.install .. '/' .. confdir
end
if config.rules then
    local rules = config.rules .. '/'
    assert(0 == eressea.config.read(rules .. 'config.json', confdir),
        "could not read JSON data")
    confdir = confdir .. rules
    catalog = confdir .. 'catalog.xml'
    if read_rules(confdir .. 'rules.dat') ~= 0 then
        assert(0 == read_xml(confdir .. 'rules.xml', catalog),
            "could not load XML data, did you compile with LIBXML2 ?")
    end
    read_xml(confdir .. 'config.xml', catalog)
    read_xml(confdir .. 'locales.xml', catalog)
end
eressea.game.reset()
