// Crew Chief wants to refer to tyres as Soft, Hard, Wet etc. but rFactor uses
// names that are defined in the vehicle data files (the *.tbc file).
// This handles the translation both ways
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace PitMenuAPI
{
  /// <summary>
  /// PitMenuAPI consists PitMenuAbstractionLayer : PitMenuController : PitMenu
  /// </summary>
  public class PitMenuAbstractionLayer : PitMenuController
  {
    private Dictionary<string, List<string>> menuDict;
    PitMenuController Pmc;

    /// <summary>
    /// All the Pit Menu categories of tyres that rF2 selects from
    /// </summary>
    private readonly string[] tyres = {
            "RR TIRE:",
            "RL TIRE:",
            "FR TIRE:",
            "FL TIRE:",
            "R TIRES:",
            "F TIRES:",
            "RT TIRES:",
            "LF TIRES:"
        };
    /// <summary>
    /// The Pit Menu categories of tyres that rF2 uses to select compounds,
    /// the remainder sometimes only choose this compound or NO CHANGE
    /// </summary>
    private readonly string[] tyreSelectionCategories = {
            "FR TIRE:",
            "FL TIRE:",
            "F TIRES:",
            "RT TIRES:",
            "LF TIRES:"
        };

    /// <summary>
    /// Disconnect from the Shared Memory running in rFactor
    /// </summary>
    public void Disconnect()
    {
      Pmc.Disconnect();
    }

    /// <summary>
    /// Get a list of the front tyre changes provided for this vehicle.  Fronts
    /// sometimes have to be changed before the rears will be given the same
    /// set of compounds available
    /// </summary>
    /// <returns>
    /// A list of the front tyre changes provided for this vehicle
    /// </returns>
    public List<string> GetFrontTyres()
    {
      return (List<string>)tyreSelectionCategories.Intersect(menuDict.Keys.ToList());
    }

    public void GetMenuDict()
    {
      Pmc.Connect();
      menuDict = Pmc.GetMenuDict();
    }

    /// <summary>
    /// Get the type of tyre selected
    /// </summary>
    /// <returns>
    /// Supersoft
    /// Soft
    /// Medium
    /// Hard
    /// Intermediate
    /// Wet
    /// Monsoon
    /// No Change
    /// </returns>
    public string GetGenericTyreType()
    {
      //if (this.GetCategory().Contains("TIRE"))
      string current = GetChoice();
      string result = "NO_TYRE";
      foreach (var genericTyreType in SampleTyreDict)
      {
        if (genericTyreType.Value.Contains(current))
        {
          result = genericTyreType.Key;
          break;
        }
      }
      return result;
    }

    /// <summary>
    /// Take a list of tyre types available in the menu and map them on to
    /// the set of generic tyre types
    /// Supersoft
    /// Soft
    /// Medium
    /// Hard
    /// Intermediate
    /// Wet
    /// Monsoon
    /// (No Change) for completeness
    /// </summary>
    /// <param name="inMenu">
    /// The list returned by GetTyreTypes()
    /// </param>
    /// <returns>
    /// Dictionary mapping generic tyre types to names of those available
    /// </returns>

    // Complicated because rF2 has many names for tyres so use a dict of
    // possible alternative names for each type
    // Each entry has a list of possible matches in declining order
    // Sample:
    public static readonly Dictionary<string, List<string>> SampleTyreDict =
      new Dictionary<string, List<string>>() {
            { "Supersoft",    new List <string> {"supersoft", "soft",
                                                  "s310", "slick", "dry", "all-weather" } },
            { "Soft",         new List <string> {"soft",
                                                  "s310", "slick", "dry", "all-weather" } },
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
    public Dictionary<string, string> TranslateTyreTypes(
      Dictionary<string, List<string>> tyreDict,
      List<string> inMenu)
    {
      Dictionary<string, string> result = new Dictionary<string, string>();
      foreach (var genericTyretype in tyreDict)
      { // "Supersoft", "Soft"...
        foreach (var availableTyretype in inMenu)
        {  // Tyre type in the menu
          foreach (var tyreName in genericTyretype.Value)
          { // Type that generic type can match to
            if (availableTyretype.IndexOf(tyreName, StringComparison.OrdinalIgnoreCase) >= 0)
            {
              result[genericTyretype.Key] = availableTyretype;
              break;
            }
          }
        }
      }
      return result;
    }

    // Unit Test
    public void setMenuDict(Dictionary<string, List<string>> dict)
    {
      menuDict = dict;
    }
  }
}
