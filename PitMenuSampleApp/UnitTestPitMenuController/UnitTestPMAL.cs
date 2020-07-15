using System;
using System.Collections.Generic;
using System.Text.Json;
using System.Web.Script.Serialization;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using PitMenuAPI;

namespace csUnitTestPitMenuAbstractionLayer
{
  using Pmal = PitMenuAbstractionLayer;
  [TestClass]
  public class UnitTest
  {
    private PitMenuController Pmc;

    ///////////////////////////////////////////////////////////////////////////
    /// Test data
    ///
    // Complicated because rF2 has many names for tyres so use a dict of
    // possible alternative names for each type
    // Each entry has a list of possible matches in declining order
    // (Copy of PitMenuAbstractionLayer.SampleTyreDict but could be edited for
    // test purposes)
    private static readonly Dictionary<string, List<string>> _tyreDict =
      new Dictionary<string, List<string>>() {
            { "Supersoft",    new List <string> {"supersoft", "soft",
                                                  "s310", "slick", "dry", "all-weather", "medium" } },
            { "Soft",         new List <string> {"soft",
                                                  "s310", "slick", "dry", "all-weather", "medium" } },
            { "Medium",       new List <string> { "medium", "default",
                                                  "s310", "slick", "dry", "all-weather" } },
            { "Hard",         new List <string> {"hard", "p310", "endur",
                                                  "medium", "default",
                                                          "slick", "dry", "all-weather" } },
            { "Intermediate", new List <string> { "intermediate",
                                                  "wet", "rain", "monsoon", "all-weather" } },
            { "Wet",          new List <string> {
                                                  "wet", "rain", "monsoon", "all-weather", "intermediate" } },
            { "Monsoon",      new List <string> {"monsoon",
                                                  "wet", "rain",  "all-weather", "intermediate" } },
            { "No Change",    new List <string> {"no change"} }
        };

    static readonly string KartDict = @"
    {
      ""R TIRES:"": [
        ""YMW WET"",
        ""No Change""
      ],
      ""FL PRESS:"": [],
      ""FR PRESS:"": [],
      ""RL PRESS:"": [],
      ""RR PRESS:"": [],
      ""FUEL:"": [],
      ""F TIRES:"": [
        ""YMW WET"",
        ""YMW DRY"",
        ""No Change""
      ]
      }";
    static readonly string StockCarDict = @"
    {
      ""FUEL:"": [],
      ""LF TIRES:"": [
        ""Speedway"",
        ""No Change""
      ],
      ""RT TIRES:"": [
        ""Speedway"",
        ""No Change""
      ],
      ""WEDGE:"": [
        ""-1.00 turns"",
        ""-0.75 turns""
      ],
      ""GRILLE:"": [
        ""61"",
        ""62 (\u002B1)""
      ],
      ""TRACK BAR:"": [
        ""27.6 (\u002B0.6)"",
        ""176 (\u002B3)""
      ],
      ""FL PRESS:"": [],
      ""FR PRESS:"": [],
      ""RL PRESS:"": [],
      ""RR PRESS:"": [],
      ""FL RUBBER:"": [
        ""0"",
        ""4 (\u002B4)""
      ],
      ""FR RUBBER:"": [
        ""9 (\u002B9)"",
        ""4 (\u002B4)""
      ],
      ""RL RUBBER:"": [
        ""4 (\u002B4)""
      ]
    }";
    ///////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////
    /// Test function
    private Dictionary<string, List<string>> _getTestDict(string json)
    {
      var serializer = new JavaScriptSerializer();

      Dictionary<string, List<string>> testDict =
        serializer.Deserialize<Dictionary<string, List<string>>>(json);
      return testDict;
    }
    ///////////////////////////////////////////////////////////////////////////

    [TestInitialize]
    public void testInit()
    {
      Pmc = new PitMenuController();
    }

    /// <summary>
    /// Test the test data
    /// </summary>
    [TestMethod]
    public void Test_getTestDict()
    {
      Dictionary<string, List<string>> result = this._getTestDict(KartDict);
      Assert.IsFalse(result.Count == 0);
      Assert.AreEqual("YMW WET", result["F TIRES:"][0]);

      var options = new JsonSerializerOptions
      {
        WriteIndented = true,
      };
      string jsonString = JsonSerializer.Serialize(result, options);
      Console.WriteLine(jsonString);

      result = this._getTestDict(StockCarDict);
      Assert.IsFalse(result.Count == 0);
      Assert.AreEqual("0", result["FL RUBBER:"][0]);

    }

    [TestMethod]
    public void Test_translateTyreTypes()
    {
      Dictionary<string, string> translation;
      List<string> menuTyres;

      translation = Pmal.TranslateTyreTypes(_tyreDict,
        _getTestDict(KartDict)["F TIRES:"]);
      Assert.AreEqual(_tyreDict.Count, translation.Count);
      Assert.AreEqual("YMW DRY", translation["Soft"]);

      menuTyres = new List<string>
      {
        "Wet",
        "Intermediate",
        "Medium",
        "Hard",
        "No Change"
      };
      translation = Pmal.TranslateTyreTypes(_tyreDict,
        menuTyres);
      Assert.AreEqual(_tyreDict.Count, translation.Count);
      Assert.AreEqual("Medium", translation["Soft"]);
    }

    [TestMethod]
    public void translateBasicTyreTypes()
    {
      List<string> tyretypesInMenu = new List<string> {
        "Soft", "medium", "Hard", "Wet" };
      Dictionary<string, string> result =
        Pmal.TranslateTyreTypes(_tyreDict, tyretypesInMenu);
      Assert.AreEqual("medium", result["Medium"]);
      Assert.AreEqual("Soft", result["Supersoft"]);
      Assert.AreEqual("Wet", result["Intermediate"]);
      Assert.AreEqual("Wet", result["Monsoon"]);
    }

    [TestMethod]
    public void translateTyreTypes()
    {
      List<string> tyretypesInMenu = new List<string> {
        "Soft", "Michelin Slick S9D", "Dunlop Hard", "Intermediate" };
      Dictionary<string, string> result =
        Pmal.TranslateTyreTypes(_tyreDict, tyretypesInMenu);
      Assert.AreEqual("Michelin Slick S9D", result["Medium"]);
      Assert.AreEqual("Michelin Slick S9D", result["Supersoft"]);
      Assert.AreEqual("Dunlop Hard", result["Hard"]);
      Assert.AreEqual("Intermediate", result["Intermediate"]);
      Assert.AreEqual("Intermediate", result["Monsoon"]);
    }

    [TestMethod]
    public void TestGetMenuDict()
    {
      if (Pmal.Connect())
      {
        Dictionary<string, List<string>> result = Pmc.GetMenuDict();
        Assert.IsFalse(result.Count == 0);
        var options = new JsonSerializerOptions
        {
          WriteIndented = true,
        };
        string jsonString = JsonSerializer.Serialize(result, options);
        Console.WriteLine(jsonString);
      }
    }

    [TestMethod]
    public void TestGetFrontTyreCategories()
    {
      Pmal.setMenuDict(_getTestDict(KartDict));

      var ftc = Pmal.GetFrontTyreCategories();
      Assert.AreEqual("F TIRES:", ftc[0]);
    }
  }
}