using System;
using System.Collections.Generic;
using Microsoft.VisualStudio.TestTools.UnitTesting;
using rF2SharedMemoryAPI;

namespace UnitTestPitMenuController
{
  [TestClass]
  public class UnitTest
  {
    private PitMenuController Pmc;

    [TestInitialize]
    public void testInit()
    {
      Pmc = new PitMenuController();
    }

    [TestMethod]
    public void translateBasicTyreTypes()
    {
      List<string> tyretypesInMenu = new List<string> {
        "Soft", "medium", "Hard", "Wet" };
      Dictionary<string, string> result =
        Pmc.translateTyreTypes(tyretypesInMenu);
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
        Pmc.translateTyreTypes(tyretypesInMenu);
      Assert.AreEqual("Michelin Slick S9D", result["Medium"]);
      Assert.AreEqual("Michelin Slick S9D", result["Supersoft"]);
      Assert.AreEqual("Dunlop Hard", result["Hard"]);
      Assert.AreEqual("Intermediate", result["Intermediate"]);
      Assert.AreEqual("Intermediate", result["Monsoon"]);
    }
  }
}
