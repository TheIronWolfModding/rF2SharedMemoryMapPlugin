/*
Set the rFactor 2 Pit Menu using TheIronWolf's rF2 Shared Memory Map plugin
https://github.com/TheIronWolfModding/rF2SharedMemoryMapPlugin

Crew Chief wants to refer to tyres as Soft, Hard, Wet etc. but rFactor uses
names that are defined in the vehicle data files (the *.tbc file).
This handles the translation both ways

Author: Tony Whitley (sven.smiles@gmail.com)
*/
using System.Collections.Generic;
using System.Linq;

namespace PitMenuAPI
{
    /// <summary>
    /// PitMenuAPI consists PitMenuAbstractionLayer : PitMenuController : PitMenu
    /// </summary>
    public static class PitMenuAbstractionLayer // : PitMenuController
    {
        #region Public Fields

        public static PitMenuController Pmc = new PitMenuController();

        #endregion Public Fields

        //static MenuLayout menuLayout = new MenuLayout();

        #region Private Fields

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

        private static readonly string[] leftTyreCategories = {
            "FL TIRE:",
            "RL TIRE:",
            "LF TIRES:",
        };

        private static readonly string[] rightTyreCategories = {
            "FR TIRE:",
            "RR TIRE:",
            "RT TIRES:",
        };

        #endregion Private Fields

        #region Public Methods

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
        /// A sorted list of the front tyre changes provided for this vehicle
        /// </returns>
        public static List<string> GetFrontTyreCategories()
        {
            List<string> result =
                frontTyreCategories.Intersect(MenuLayout.getKeys()).ToList();
            result.Sort();
            return result;
        }

        /// <summary>
        /// Get a list of all the tyre changes provided for this vehicle.
        /// </summary>
        /// <returns>
        /// A sorted list of all the tyre changes provided for this vehicle
        /// </returns>
        public static List<string> GetAllTyreCategories()
        {
            List<string> result =
                tyreCategories.Intersect(MenuLayout.getKeys()).ToList();
            result.Sort();
            return result;
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

        public static List<string> GetLeftTyreCategories()
        {
            return leftTyreCategories.Intersect(MenuLayout.getKeys()).ToList();
        }

        public static List<string> GetRightTyreCategories()
        {
            return rightTyreCategories.Intersect(MenuLayout.getKeys()).ToList();
        }

        public static List<string> GetTyreTypeNames()
        {
            string tyre = GetFrontTyreCategories()[0];
            return MenuLayout.get(tyre);
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

        #endregion Public Methods

        #region Private Classes

        /// <summary>
        /// Virtualisation of the menu layout for the current vehicle
        /// </summary>
        private static class MenuLayout
        {
            #region Private Fields

            private static Dictionary<string, List<string>> menuDict =
                new Dictionary<string, List<string>>();

            #endregion Private Fields

            #region Public Methods

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

            #endregion Public Methods
        }

        #endregion Private Classes
    }
}