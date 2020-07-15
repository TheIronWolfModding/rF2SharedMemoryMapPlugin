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
    public static class PitMenuAbstractionLayer // : PitMenuController
    {
        static PitMenuController Pmc = new PitMenuController();
        //static MenuLayout menuLayout = new MenuLayout();

        /// <summary>
        /// All the Pit Menu categories of tyres that rF2 selects from
        /// </summary>
        private static readonly string[] tyreCategories = {
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
        private static readonly string[] frontTyreCategories = {
            "FR TIRE:",
            "FL TIRE:",
            "F TIRES:",
            "RT TIRES:",
            "LF TIRES:",
            "TIRES:"
        };

        /// <summary>
        /// Virtualisation of the menu layout for the current vehicle
        /// </summary>
        private static class MenuLayout
        {
            static Dictionary<string, List<string>> menuDict =
                new Dictionary<string, List<string>>();
            public static void NewCar()
            {
                menuDict = new Dictionary<string, List<string>> { };
            }
            public static List<string> get(string key)
            {
                List<string> value;
                if (menuDict.Count == 0)
                {
                    menuDict = Pmc.GetMenuDict();
                }
                if (menuDict.TryGetValue(key, out value))
                {
                    return value;
                }
                return new List<string>();
            }
            public static List<string> getKeys()
            {
                if (menuDict.Count == 0)
                {
                    menuDict = Pmc.GetMenuDict();
                }
                return new List<string>(menuDict.Keys);
            }
            public static void set(Dictionary<string, List<string>> unitTestDict)
            {
                menuDict = unitTestDict;
            }
        }

        /// <summary>
        /// Connect to the Shared Memory running in rFactor
        /// </summary>
        public static bool Connect()
        {
            return Pmc.Connect();
        }
        /// <summary>
        /// Disconnect from the Shared Memory running in rFactor
        /// </summary>
        public static void Disconnect()
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
        public static List<string> GetFrontTyreCategories()
        {
            return frontTyreCategories.Intersect(MenuLayout.getKeys()).ToList();
        }

        public static string GetCurrentTyreType()
        {
            string result;
            foreach (string category in GetFrontTyreCategories())
            {
                Pmc.SetCategory(category);
                result = Pmc.GetChoice();
                if (result != "No Change")
                {
                    return result;
                }
            }
            foreach (string category in GetRearTyreCategories())
            {
                Pmc.SetCategory(category);
                result = Pmc.GetChoice();
                if (result != "No Change")
                {
                    return result;
                }
            }
            return "No Change";
        }

        /// <summary>
        /// Get a list of the rear tyre changes provided for this vehicle.
        /// </summary>
        /// <returns>
        /// A list of the rear tyre changes provided for this vehicle
        /// </returns>
        public static List<string> GetRearTyreCategories()
        {
            // There are simpler ways to do this but...
            return (List<string>)tyreCategories.Except(frontTyreCategories)
              .Intersect(MenuLayout.getKeys()).ToList();
        }
        public static void GetMenuDict()
        {
            //Pmc.Connect();
            //menuDict = Pmc.GetMenuDict();
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
        public static string GetGenericTyreType()
        {
            //if (this.GetCategory().Contains("TIRE"))
            string current = Pmc.GetChoice();
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

        public static List<string> GetTyreTypeNames()
        {
            string tyre = GetFrontTyreCategories()[0];
            return MenuLayout.get(tyre);
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
        public static Dictionary<string, string> TranslateTyreTypes(
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
        public static bool SetAllTyreTypes(string tyreType)
        {
            bool response = true;

            foreach (string whichTyre in GetFrontTyreCategories())
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

        public static bool setCategoryAndChoice(string category, string choice)
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
        public static void setMenuDict(Dictionary<string, List<string>> dict)
        {
            MenuLayout.set(dict);
        }
    }
}
