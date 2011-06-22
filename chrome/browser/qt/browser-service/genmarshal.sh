#!/bin/sh
echo "Generating marshaller...."
glib-genmarshal --prefix=browser_service_marshal browser/qt/browser-service/marshal.list --header > browser/qt/browser-service/BrowserService-marshaller.h

echo "#include \"BrowserService-marshaller.h\"" > browser/qt/browser-service/BrowserService-marshaller.cpp
glib-genmarshal --prefix=browser_service_marshal browser/qt/browser-service/marshal.list --body >> browser/qt/browser-service/BrowserService-marshaller.cpp

echo "Generating glue code ..."
dbus-binding-tool --prefix=browser_service --mode=glib-server --output=browser/qt/browser-service/BrowserService-glue.h browser/qt/browser-service/BrowserService.xml
