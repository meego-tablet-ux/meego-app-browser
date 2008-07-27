description(
'This tests enumerating the elements of DOM lists.'
);

if (window.layoutTestController)
    layoutTestController.dumpAsText();

// Create a testing environment that can be cleanup up easily.
var testingGround = document.createElement('div');
document.body.appendChild(testingGround);

function createFromMarkup(markup)
{
    var range = document.createRange();
    var fragmentContainer = document.createElement("div");
    range.selectNodeContents(fragmentContainer);
    testingGround.appendChild(fragmentContainer);
    var fragment = range.createContextualFragment(markup);
    fragmentContainer.appendChild(fragment);
    return fragmentContainer.firstChild;
}

function setup()
{
    var head = document.getElementsByTagName('head')[0];

    // 2 additional <style>s needed for StyleSheetList test (one is included in the template).
    // 3 rules needed in the first addtional <style> for the CSSRuleList test.
    // 3 declarations needed in the first rule needed for the CSSStyleDeclaration test.
    // @media rule in the second addtional <style> for the MediaList test.
    head.appendChild(createFromMarkup("<style> ol { width: 100px; height: 100px; color: green; } ol { } ol { } </style>"));
    head.appendChild(createFromMarkup("<style> @media screen, projector, printer { ol { } } </style>"));

    // 3 <ol>s for NodeList test.
    // 3 attributes in the first <ol> for the NamedNodeMap test.
    testingGround.appendChild(createFromMarkup("<ol class='foo' id='bar' name='baz'></ol>"));
    testingGround.appendChild(document.createElement('ol'));
    testingGround.appendChild(document.createElement('ol'));

    // 3 <form>s for forms for HTMLCollection test.
    var form = document.createElement('form');
    testingGround.appendChild(form);
    testingGround.appendChild(document.createElement('form'));
    testingGround.appendChild(document.createElement('form'));

    // 3 <select>s for HTMLFormElement test.
    var select = document.createElement('select');
    form.appendChild(select);
    form.appendChild(document.createElement('select'));
    form.appendChild(document.createElement('select'));

    // 3 <option>s for HTMLSelectElement test.
    select.appendChild(document.createElement('option'));
    select.appendChild(document.createElement('option'));
    select.appendChild(document.createElement('option'));

    document.body.appendChild(testingGround);
}

function iterateList(list)
{
    debug("");
    debug(Object.prototype.toString.call(list));
    var a = new Array();
    for (var i in list) {
      a.push({"i" : i, "item" : list[i]});
    }
    a.sort(function (a, b) { 
        var a_string = a.i.toString();
        var b_string = b.i.toString();
        var len = a_string.length < b_string.length ? a_string.length : b_string.length;
        for (var i = 0; i < len; i++) {
          if (a_string.charCodeAt(i) < b_string.charCodeAt(i)) return -1;
          if (a_string.charCodeAt(i) > b_string.charCodeAt(i)) return 1;
        }
        return a_string.length - b_string.length;
      });
    return a;
}

// ** Firefox DOES include the indexGetter results in enumeration **
// NodeList
// HTMLCollection
// CSSRuleList
// CSSStyleDeclaration
// CSSValueList
// StyleSheetList
// MediaList
// NamedNodeMap
// HTMLFormElement
// HTMLSelectElement

// ** Firefox DOESN'T include the indexGetter results in enumeration **
// Window

setup();

var resultArray = new Array();

// NodeList
var nodeList = document.getElementsByTagName('ol');
resultArray = iterateList(nodeList);
shouldBe("resultArray.length", "5");
shouldBe("resultArray[0].i", "'0'");
shouldBe("resultArray[0].item", "nodeList.item(0)");
shouldBe("resultArray[1].i", "'1'");
shouldBe("resultArray[1].item", "nodeList.item(1)");
shouldBe("resultArray[2].i", "'2'");
shouldBe("resultArray[2].item", "nodeList.item(2)");
shouldBe("resultArray[3].i", "'item'");
shouldBe("resultArray[3].item", "nodeList.item");
shouldBe("resultArray[4].i", "'length'");
shouldBe("resultArray[4].item", "nodeList.length");

// HTMLCollection
var htmlCollection = document.forms;
resultArray = iterateList(htmlCollection);
shouldBe("resultArray.length", "7");
shouldBe("resultArray[0].i", "'0'");
shouldBe("resultArray[0].item", "htmlCollection.item(0)");
shouldBe("resultArray[1].i", "'1'");
shouldBe("resultArray[1].item", "htmlCollection.item(1)");
shouldBe("resultArray[2].i", "'2'");
shouldBe("resultArray[2].item", "htmlCollection.item(2)");
shouldBe("resultArray[3].i", "'item'");
shouldBe("resultArray[3].item", "htmlCollection.item");
shouldBe("resultArray[4].i", "'length'");
shouldBe("resultArray[4].item", "htmlCollection.length");
shouldBe("resultArray[5].i", "'namedItem'");
shouldBe("resultArray[5].item", "htmlCollection.namedItem");
shouldBe("resultArray[6].i", "'tags'");
shouldBe("resultArray[6].item", "htmlCollection.tags");

// NamedNodeMap
var namedNodeMap = document.getElementsByTagName('ol')[0].attributes;
resultArray = iterateList(namedNodeMap);
shouldBe("resultArray.length", "11");
shouldBe("resultArray[0].i", "'0'");
shouldBe("resultArray[0].item", "namedNodeMap.item(0)");
shouldBe("resultArray[1].i", "'1'");
shouldBe("resultArray[1].item", "namedNodeMap.item(1)");
shouldBe("resultArray[2].i", "'2'");
shouldBe("resultArray[2].item", "namedNodeMap.item(2)");
shouldBe("resultArray[3].i", "'getNamedItem'");
shouldBe("resultArray[3].item", "namedNodeMap.getNamedItem");
shouldBe("resultArray[4].i", "'getNamedItemNS'");
shouldBe("resultArray[4].item", "namedNodeMap.getNamedItemNS");
shouldBe("resultArray[5].i", "'item'");
shouldBe("resultArray[5].item", "namedNodeMap.item");
shouldBe("resultArray[6].i", "'length'");
shouldBe("resultArray[6].item", "namedNodeMap.length");
shouldBe("resultArray[7].i", "'removeNamedItem'");
shouldBe("resultArray[7].item", "namedNodeMap.removeNamedItem");
shouldBe("resultArray[8].i", "'removeNamedItemNS'");
shouldBe("resultArray[8].item", "namedNodeMap.removeNamedItemNS");
shouldBe("resultArray[9].i", "'setNamedItem'");
shouldBe("resultArray[9].item", "namedNodeMap.setNamedItem");
shouldBe("resultArray[10].i", "'setNamedItemNS'");
shouldBe("resultArray[10].item", "namedNodeMap.setNamedItemNS");

// HTMLFormElement
var htmlFormElement = document.getElementsByTagName('form')[0];
resultArray = iterateList(htmlFormElement);
shouldBe("resultArray.length", "114");
shouldBe("resultArray[0].i", "'0'");
shouldBe("resultArray[0].item", "document.getElementsByTagName('select')[0]");
shouldBe("resultArray[1].i", "'1'");
shouldBe("resultArray[1].item", "document.getElementsByTagName('select')[1]");
shouldBe("resultArray[2].i", "'2'");
shouldBe("resultArray[2].item", "document.getElementsByTagName('select')[2]");
shouldBe("resultArray[3].i", "'ATTRIBUTE_NODE'");
shouldBe("resultArray[3].item", "htmlFormElement.ATTRIBUTE_NODE");
debug("...ellided the remaining 109");

// HTMLSelectElement
var htmlSelectElement = document.getElementsByTagName('select')[0];
resultArray = iterateList(htmlSelectElement);
shouldBe("resultArray.length", "117");
shouldBe("resultArray[0].i", "'0'");
shouldBe("resultArray[0].item", "document.getElementsByTagName('option')[0]");
shouldBe("resultArray[1].i", "'1'");
shouldBe("resultArray[1].item", "document.getElementsByTagName('option')[1]");
shouldBe("resultArray[2].i", "'2'");
shouldBe("resultArray[2].item", "document.getElementsByTagName('option')[2]");
shouldBe("resultArray[3].i", "'ATTRIBUTE_NODE'");
shouldBe("resultArray[3].item", "htmlSelectElement.ATTRIBUTE_NODE");
debug("...ellided the remaining 113");

// StyleSheetList
var styleSheetList = document.styleSheets;
resultArray = iterateList(styleSheetList);
shouldBe("resultArray.length", "5");
shouldBe("resultArray[0].i", "'0'");
shouldBe("resultArray[0].item", "styleSheetList.item(0)");
shouldBe("resultArray[1].i", "'1'");
shouldBe("resultArray[1].item", "styleSheetList.item(1)");
shouldBe("resultArray[2].i", "'2'");
shouldBe("resultArray[2].item", "styleSheetList.item(2)");
shouldBe("resultArray[3].i", "'item'");
shouldBe("resultArray[3].item", "styleSheetList.item");
shouldBe("resultArray[4].i", "'length'");
shouldBe("resultArray[4].item", "styleSheetList.length");

// CSSRuleList
var cssRuleList = document.styleSheets[1].cssRules;
resultArray = iterateList(cssRuleList);
shouldBe("resultArray.length", "5");
shouldBe("resultArray[0].i", "'0'");
shouldBe("resultArray[0].item", "cssRuleList.item(0)");
shouldBe("resultArray[1].i", "'1'");
shouldBe("resultArray[1].item", "cssRuleList.item(1)");
shouldBe("resultArray[2].i", "'2'");
shouldBe("resultArray[2].item", "cssRuleList.item(2)");
shouldBe("resultArray[3].i", "'item'");
shouldBe("resultArray[3].item", "cssRuleList.item");
shouldBe("resultArray[4].i", "'length'");
shouldBe("resultArray[4].item", "cssRuleList.length");

// CSSStyleDeclaration
//debug(escapeHTML(document.getElementsByTagName('style')));
var cssStyleDeclaration = document.styleSheets[1].cssRules[0].style;
resultArray = iterateList(cssStyleDeclaration);
shouldBe("resultArray.length", "14");
shouldBe("resultArray[0].i", "'0'");
shouldBe("resultArray[0].item", "cssStyleDeclaration.item(0)");
shouldBe("resultArray[1].i", "'1'");
shouldBe("resultArray[1].item", "cssStyleDeclaration.item(1)");
shouldBe("resultArray[2].i", "'2'");
shouldBe("resultArray[2].item", "cssStyleDeclaration.item(2)");
shouldBe("resultArray[3].i", "'cssText'");
shouldBe("resultArray[3].item", "cssStyleDeclaration.cssText");
shouldBe("resultArray[4].i", "'getPropertyCSSValue'");
shouldBe("resultArray[4].item", "cssStyleDeclaration.getPropertyCSSValue");
shouldBe("resultArray[5].i", "'getPropertyPriority'");
shouldBe("resultArray[5].item", "cssStyleDeclaration.getPropertyPriority");
shouldBe("resultArray[6].i", "'getPropertyShorthand'");
shouldBe("resultArray[6].item", "cssStyleDeclaration.getPropertyShorthand");
shouldBe("resultArray[7].i", "'getPropertyValue'");
shouldBe("resultArray[7].item", "cssStyleDeclaration.getPropertyValue");
shouldBe("resultArray[8].i", "'isPropertyImplicit'");
shouldBe("resultArray[8].item", "cssStyleDeclaration.isPropertyImplicit");
shouldBe("resultArray[9].i", "'item'");
shouldBe("resultArray[9].item", "cssStyleDeclaration.item");
shouldBe("resultArray[10].i", "'length'");
shouldBe("resultArray[10].item", "cssStyleDeclaration.length");
shouldBe("resultArray[11].i", "'parentRule'");
shouldBe("resultArray[11].item", "cssStyleDeclaration.parentRule");
shouldBe("resultArray[12].i", "'removeProperty'");
shouldBe("resultArray[12].item", "cssStyleDeclaration.removeProperty");
shouldBe("resultArray[13].i", "'setProperty'");
shouldBe("resultArray[13].item", "cssStyleDeclaration.setProperty");

// CSSValueList
var cssValueList = window.getComputedStyle(document.getElementsByTagName('ol')[0]).getPropertyCSSValue('border-spacing');
resultArray = iterateList(cssValueList);
shouldBe("resultArray.length", "10");
shouldBe("resultArray[0].i", "'0'");
shouldBe("resultArray[0].item", "cssValueList.item(0)");
shouldBe("resultArray[1].i", "'1'");
shouldBe("resultArray[1].item", "cssValueList.item(1)");
shouldBe("resultArray[2].i", "'CSS_CUSTOM'");
shouldBe("resultArray[2].item", "cssValueList.CSS_CUSTOM");
shouldBe("resultArray[3].i", "'CSS_INHERIT'");
shouldBe("resultArray[3].item", "cssValueList.CSS_INHERIT");
shouldBe("resultArray[4].i", "'CSS_PRIMITIVE_VALUE'");
shouldBe("resultArray[4].item", "cssValueList.CSS_PRIMITIVE_VALUE");
shouldBe("resultArray[5].i", "'CSS_VALUE_LIST'");
shouldBe("resultArray[5].item", "cssValueList.CSS_VALUE_LIST");
shouldBe("resultArray[6].i", "'cssText'");
shouldBe("resultArray[6].item", "cssValueList.cssText");
shouldBe("resultArray[7].i", "'cssValueType'");
shouldBe("resultArray[7].item", "cssValueList.cssValueType");
shouldBe("resultArray[8].i", "'item'");
shouldBe("resultArray[8].item", "cssValueList.item");
shouldBe("resultArray[9].i", "'length'");
shouldBe("resultArray[9].item", "cssValueList.length");

// MediaList
var mediaList = document.styleSheets[2].cssRules[0].media;
resultArray = iterateList(mediaList);
shouldBe("resultArray.length", "8");
shouldBe("resultArray[0].i", "'0'");
shouldBe("resultArray[0].item", "mediaList.item(0)");
shouldBe("resultArray[1].i", "'1'");
shouldBe("resultArray[1].item", "mediaList.item(1)");
shouldBe("resultArray[2].i", "'2'");
shouldBe("resultArray[2].item", "mediaList.item(2)");
shouldBe("resultArray[3].i", "'appendMedium'");
shouldBe("resultArray[3].item", "mediaList.appendMedium");
shouldBe("resultArray[4].i", "'deleteMedium'");
shouldBe("resultArray[4].item", "mediaList.deleteMedium");
shouldBe("resultArray[5].i", "'item'");
shouldBe("resultArray[5].item", "mediaList.item");
shouldBe("resultArray[6].i", "'length'");
shouldBe("resultArray[6].item", "mediaList.length");
shouldBe("resultArray[7].i", "'mediaText'");
shouldBe("resultArray[7].item", "mediaList.mediaText");

debug("");

document.body.removeChild(testingGround);

var successfullyParsed = true;
