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
    PitMenuController Pmc = new PitMenuController();

    /// <summary>
    /// All the Pit Menu categories of tyres that rF2 selects from
    /// </summary>
    private readonly string[] tyreCategories = {
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
    private readonly string[] frontTyreCategories = {
            "FR TIRE:",
            "FL TIRE:",
            "F TIRES:",
            "RT TIRES:",
            "LF TIRES:",
            "TIRES:"
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
    public List<string> GetFrontTyreCategories()
    {
      return frontTyreCategories.Intersect(menuDict.Keys).ToList();
    }

    /// <summary>
    /// Get a list of the rear tyre changes provided for this vehicle.
    /// </summary>
    /// <returns>
    /// A list of the rear tyre changes provided for this vehicle
    /// </returns>
    public List<string> GetRearTyreCategories()
    {
      // There are simpler ways to do this but...
      return (List<string>)tyreCategories.Except(frontTyreCategories)
        .Intersect(menuDict.Keys).ToList();
    }
    public void GetMenuDict()
    {
      //Pmc.Connect();
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

    public List<string> GetTyreTypeNames()
    {
      //Pmc.SetCategory(GetFrontTyreCategories()[0]);
      return Pmc.GetTyreTypeNames();
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

    /// <summary>
    /// Set the tyre compound selection in the Pit Menu.
    /// Set the front tyres first as the rears may may depend on what is
    /// selected for the fronts
    /// Having changed them all, the client can then set specific tyres to
    /// NO CHANGE
    /// </summary>
    /// <param name="tyreType">Name of actual tyre type or NO CHANGE</param>
    /// <returns>true all tyres changed</returns>
    public bool SetAllTyreTypes(string tyreType)
    {
      bool response = true;

      foreach (string whichTyre in GetFrontTyreCategories())
      {
        if (response)
        {
          response = Pmc.SetCategory(whichTyre);
        }
        if(response)
        {
          response = Pmc.SetTyreType(tyreType);
        }
      }
      foreach (string whichTyre in GetRearTyreCategories())
      {
        if (response)
        {
          response = Pmc.SetCategory(whichTyre);
        }
        if (response)
        {
          response = Pmc.SetTyreType(tyreType);
        }
      }
      return response;
    }

    public bool setCategoryAndChoice(string category, string choice)
    {
      int tryNo = 5;
      bool response;
      while (tryNo-- > 0)
      {
        response = Pmc.SetCategory(category);
        if (response)
        {
          response = Pmc.SetChoice(choice);
          if (response)
          {
            return true;
          }
          Pmc.startUsingPitMenu();
        }
      }
      return false;
    }

    // Unit Test
  public void setMenuDict(Dictionary<string, List<string>> dict)
    {
      menuDict = dict;
    }
  }
}
