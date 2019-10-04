package java_oc_onboarding_tool;

import java.util.Collections;
import java.util.InputMismatchException;
import java.util.LinkedHashSet;
import java.util.Scanner;
import java.util.Set;

import org.iotivity.*;
import org.iotivity.oc.*;

public class ObtMain {

    /* user input Scanner */
    private static Scanner scanner = new Scanner(System.in);

    /* Constants */
    private static final int MAX_NUM_RESOURCES = 100;
    private static final int MAX_NUM_RT = 50;

    /* Sets containing discovered owned and un-owned devices */
    static Set<OcfDeviceInfo> unownedDevices = Collections.synchronizedSet(new LinkedHashSet<OcfDeviceInfo>());
    static Set<OcfDeviceInfo> ownedDevices = Collections.synchronizedSet(new LinkedHashSet<OcfDeviceInfo>());

    /* Callback handlers */
    private static UnownedDeviceHandler unownedDeviceHandler = new UnownedDeviceHandler();
    private static OwnedDeviceHandler ownedDeviceHandler = new OwnedDeviceHandler();
    private static GenerateRandomPinHandler generateRandomPinHandler = new GenerateRandomPinHandler();
    private static ProvisionCredentialsHandler provisionCredentialsHandler = new ProvisionCredentialsHandler();
    private static ProvisionAce2Handler provisionAce2Handler = new ProvisionAce2Handler();
    private static ProvisionAuthWildcardAceHandler provisionAuthWildcardAceHandler = new ProvisionAuthWildcardAceHandler();
    private static ProvisionRoleWildcardAceHandler provisionRoleWildcardAceHandler = new ProvisionRoleWildcardAceHandler();
    private static ProvisionIdCertificateHandler provisionIdCertificateHandler = new ProvisionIdCertificateHandler();
    private static ProvisionRoleCertificateHandler provisionRoleCertificateHandler = new ProvisionRoleCertificateHandler();
    private static ResetDeviceHandler resetDeviceHandler = new ResetDeviceHandler();

    private static boolean quit;
    static OcObt obt;
    private static OcPlatform obtPlatform = OcPlatform.getInstance();
    private static Thread mainThread = Thread.currentThread();

    private static Thread shutdownHook = new Thread() {
        public void run() {
            quit = true;
            System.out.println("Calling platform shutdown.");
            obtPlatform.systemShutdown();
            obt.shutdown();
            scanner.close();
            mainThread.interrupt();
        }
    };

    private static void displayMenu() {
        StringBuilder menu = new StringBuilder();
        menu.append("\n################################################\n");
        menu.append("OCF 2.0 Onboarding Tool\n");
        menu.append("################################################\n");
        menu.append("[0] Display this menu\n");
        menu.append("------------------------------------------------\n");
        menu.append("[1] Discover un-owned devices\n");
        menu.append("[2] Discover un-owned devices in the realm-local IPv6 scope\n");
        menu.append("[3] Discover un-owned devices in the site-local IPv6 scope\n");
        menu.append("[4] Discover owned devices\n");
        menu.append("[5] Discover owned devices in the realm-local IPv6 scope\n");
        menu.append("[6] Discover owned devices in the site-local IPv6 scope\n");
        menu.append("------------------------------------------------\n");
        menu.append("[7] Just-Works Ownership Transfer Method\n");
        menu.append("[8] Request Random PIN from device for OTM\n");
        menu.append("[9] Random PIN Ownership Transfer Method\n");
        menu.append("[10] Manufacturer Certificate based Ownership Transfer Method\n");
        menu.append("------------------------------------------------\n");
        menu.append("[11] Provision pair-wise credentials\n");
        menu.append("[12] Provision ACE2\n");
        menu.append("[13] Provision auth-crypt RW access to NCRs\n");
        menu.append("[14] Provision role RW access to NCRs\n");
        menu.append("[15] Provision identity certificate\n");
        menu.append("[16] Provision role certificate\n");
        menu.append("------------------------------------------------\n");
        menu.append("[97] RESET device\n");
        menu.append("[98] RESET OBT\n");
        menu.append("------------------------------------------------\n");
        menu.append("[99] Exit\n");
        menu.append("################################################\n");
        menu.append("\nSelect option: ");
        System.out.print(menu);
    }

    private static void discoverUnownedDevices(int scope) {

        if (scope == 1) {
            System.out.println("Discovering un-owned devices realm local IPv6");
            if (obt.discoverUnownedDevicesRealmLocalIPv6(unownedDeviceHandler) < 0) {
                System.err.println("ERROR discovering un-owned devices realm local IPv6.");
            }
        } else if (scope == 2) {
            System.out.println("Discovering un-owned devices site local IPv6");
            if (obt.discoverUnownedDevicesSiteLocalIPv6(unownedDeviceHandler) < 0) {
                System.err.println("ERROR discovering un-owned devices site local IPv6.");
            }
        } else {
            System.out.println("Discovering un-owned devices");
            if (obt.discoverUnownedDevices(unownedDeviceHandler) < 0) {
                System.err.println("ERROR discovering un-owned devices.");
            }
        }
    }

    private static void discoverOwnedDevices(int scope) {
        if (scope == 1) {
            System.out.println("Discovering owned devices realm local IPv6");
            if (obt.discoverOwnedDevicesRealmLocalIPv6(ownedDeviceHandler) < 0) {
                System.err.println("ERROR discovering owned devices realm local IPv6.");
            }
        } else if (scope == 2) {
            System.out.println("Discovering owned devices site local IPv6");
            if (obt.discoverOwnedDevicesSiteLocalIPv6(ownedDeviceHandler) < 0) {
                System.err.println("ERROR discovering owned devices site local IPv6.");
            }
        } else {
            System.out.println("Discovering owned devices");
            if (obt.discoverOwnedDevices(ownedDeviceHandler) < 0) {
                System.err.println("ERROR discovering owned Devices.");
            }
        }
    }

    private static void otmJustWorks() {
        if (unownedDevices.isEmpty()) {
            System.out.println("\nPlease Re-discover Unowned devices");
            return;
        }

        int i = 0;

        StringBuilder unownedDevicesMenu = new StringBuilder();
        unownedDevicesMenu.append("\nUnowned Devices:\n");
        OcfDeviceInfo[] uds = unownedDevices.toArray(new OcfDeviceInfo[unownedDevices.size()]);
        for (OcfDeviceInfo ud : uds) {
            unownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(ud.getUuid()) + " - " + ud.getName() + "\n");
            i++;
        }
        unownedDevicesMenu.append("\n\nSelect device: ");
        System.out.print(unownedDevicesMenu);

        int userInput = scanner.nextInt();
        if (userInput < 0 || userInput >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        JustWorksHandler justWorksHandler = new JustWorksHandler(uds[userInput]);
        int ret = obt.performJustWorksOtm(uds[userInput].getUuid(), justWorksHandler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to perform ownership transfer");
        } else {
            System.out.println("\nERROR issuing request to perform ownership transfer");
        }

        /*
         * Having issued an OTM request, remove this item from the unowned
         * device list
         */
        unownedDevices.remove(uds[userInput]);
    }

    private static void requestRandomPin() {
        if (unownedDevices.isEmpty()) {
            System.out.println("\nPlease Re-discover Unowned devices");
            return;
        }

        int i = 0;

        StringBuilder unownedDevicesMenu = new StringBuilder();
        unownedDevicesMenu.append("\nUnowned Devices:\n");
        OcfDeviceInfo[] uds = unownedDevices.toArray(new OcfDeviceInfo[unownedDevices.size()]);
        for (OcfDeviceInfo ud : uds) {
            unownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(ud.getUuid()) + " - " + ud.getName() + "\n");
            i++;
        }
        unownedDevicesMenu.append("\n\nSelect device: ");
        System.out.print(unownedDevicesMenu);

        int userInput = scanner.nextInt();
        if (userInput < 0 || userInput >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        int ret = obt.requestRandomPin(uds[userInput].getUuid(), generateRandomPinHandler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to generate a random pin");
        } else {
            System.out.println("\nERROR issuing request to generate a random pin");
        }
    }

    private static void otmRandomPin() {
        if (unownedDevices.isEmpty()) {
            System.out.println("\nPlease Re-discover Unowned devices");
            return;
        }

        int i = 0;

        StringBuilder unownedDevicesMenu = new StringBuilder();
        unownedDevicesMenu.append("\nUnowned Devices:\n");
        OcfDeviceInfo[] uds = unownedDevices.toArray(new OcfDeviceInfo[unownedDevices.size()]);
        for (OcfDeviceInfo ud : uds) {
            unownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(ud.getUuid()) + " - " + ud.getName() + "\n");
            i++;
        }
        unownedDevicesMenu.append("\n\nSelect device: ");
        System.out.print(unownedDevicesMenu);

        int userInput = scanner.nextInt();
        if (userInput < 0 || userInput >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        System.out.print("Enter Random PIN: ");
        String pin = scanner.next();
        // max string length for pin is 24 characters
        if (pin.length() > 24) {
            pin = pin.substring(0, 24);
        }

        OtmRandomPinHandler otmRandomPinHandler = new OtmRandomPinHandler(uds[userInput]);
        int ret = obt.performRandomPinOtm(uds[userInput].getUuid(), pin, otmRandomPinHandler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to perform Random PIN OTM");
        } else {
            System.out.println("\nERROR issuing request to perform Random PIN OTM");
        }

        /*
         * Having issued an OTM request, remove this item from the unowned
         * device list
         */
        unownedDevices.remove(uds[userInput]);
    }

    private static void otmCertificate() {
        if (unownedDevices.isEmpty()) {
            System.out.println("\nPlease Re-discover Unowned devices");
            return;
        }

        int i = 0;

        StringBuilder unownedDevicesMenu = new StringBuilder();
        unownedDevicesMenu.append("\nUnowned Devices:\n");
        OcfDeviceInfo[] uds = unownedDevices.toArray(new OcfDeviceInfo[unownedDevices.size()]);
        for (OcfDeviceInfo ud : uds) {
            unownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(ud.getUuid()) + " - " + ud.getName() + "\n");
            i++;
        }
        unownedDevicesMenu.append("\n\nSelect device: ");
        System.out.print(unownedDevicesMenu);

        int userInput = scanner.nextInt();
        if (userInput < 0 || userInput >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        OtmCertificationHandler otmCertificateHandler = new OtmCertificationHandler(uds[userInput]);
        int ret = obt.performCertOtm(uds[userInput].getUuid(), otmCertificateHandler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to perform Certificate OTM");
        } else {
            System.out.println("\nERROR issuing request to perform Certificate OTM");
        }

        /*
         * Having issued an OTM request, remove this item from the unowned
         * device list
         */
        unownedDevices.remove(uds[userInput]);
    }

    private static void provisionCredentials() {
        if (ownedDevices.isEmpty()) {
            System.out.println("\n\nPlease Re-Discover Owned devices");
            return;
        }

        int i = 0;

        StringBuilder ownedDevicesMenu = new StringBuilder();
        ownedDevicesMenu.append("\nMy Devices:\n");
        OcfDeviceInfo[] ods = ownedDevices.toArray(new OcfDeviceInfo[ownedDevices.size()]);
        for (OcfDeviceInfo od : ods) {
            ownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(od.getUuid()) + " - " + od.getName() + "\n");
            i++;
        }
        ownedDevicesMenu.append("\nSelect device 1: ");
        System.out.print(ownedDevicesMenu);
        int userInput1 = scanner.nextInt();
        if (userInput1 < 0 || userInput1 >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        System.out.print("\nSelect device 2: ");
        int userInput2 = scanner.nextInt();
        if (userInput2 < 0 || userInput2 >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        int ret = obt.provisionPairwiseCredentials(ods[userInput1].getUuid(), ods[userInput2].getUuid(),
                provisionCredentialsHandler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to provision credentials");
        } else {
            System.out.println("\nERROR issuing request to provision credentials");
        }
    }

    private static void provisionAce2() {
        if (ownedDevices.isEmpty()) {
            System.out.println("\n\nPlease Re-Discover Owned devices");
            return;
        }

        String[] connTypes = new String[] { "anon-clear", "auth-crypt" };
        int num_resources = 0;

        System.out.println("\nProvision ACE2\nMy Devices:");

        int i = 0;

        StringBuilder ownedDevicesMenu = new StringBuilder();
        ownedDevicesMenu.append("\nMy Devices:\n");
        OcfDeviceInfo[] ods = ownedDevices.toArray(new OcfDeviceInfo[ownedDevices.size()]);
        for (OcfDeviceInfo od : ods) {
            ownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(od.getUuid()) + " - " + od.getName() + "\n");
            i++;
        }

        if (i == 0) {
            System.out.println("\nNo devices to provision... Please Re-Discover Owned devices.");
            return;
        }

        ownedDevicesMenu.append("\n\nSelect device for provisioning: ");
        System.out.print(ownedDevicesMenu);
        int dev = scanner.nextInt();
        if (dev < 0 || dev >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        StringBuilder subjectsMenu = new StringBuilder();
        subjectsMenu.append("\nSubjects:\n");
        subjectsMenu.append("[0]: " + connTypes[0] + "\n");
        subjectsMenu.append("[1]: " + connTypes[1] + "\n");
        subjectsMenu.append("[2]: Role\n");
        i = 0;
        for (OcfDeviceInfo od : ods) {
            subjectsMenu.append(
                    "[" + (i + 3) + "]: " + OCUuidUtil.uuidToString(od.getUuid()) + " - " + od.getName() + "\n");
            i++;
        }
        subjectsMenu.append("\nSelect subject: ");
        System.out.print(subjectsMenu);
        int sub = scanner.nextInt();

        if (sub >= (i + 3)) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        OcSecurityAce ace = null;
        if (sub > 2) {
            ace = new OcSubjectSecurityAce(ods[sub - 3].getUuid());
        } else {
            if (sub == 0) {
                ace = new OcAnonSecurityAce();
            } else if (sub == 1) {
                ace = new OcAuthSecurityAce();
            } else {
                System.out.println("\nEnter role: ");
                String role = scanner.next();
                // max string length for role is 64 characters
                if (role.length() > 64) {
                    role = role.substring(0, 64);
                }
                String authority = null;
                System.out.print("Authority? [0-No, 1-Yes]: ");
                int c = scanner.nextInt();
                if (c == 1) {
                    System.out.println("\nEnter authority: ");
                    authority = scanner.next();
                    // max string length for role is 64 characters
                    if (authority.length() > 64) {
                        authority = authority.substring(0, 64);
                    }
                }
                ace = new OcRoleSecurityAce(role, authority);
            }
        }

        if (ace == null) {
            System.out.println("\nERROR: Could not create ACE");
            return;
        }

        while (num_resources <= 0 || num_resources > MAX_NUM_RESOURCES) {
            if (num_resources != 0) {
                System.out.println("\n\nERROR: Enter valid number\n");
            }
            System.out.print("\nEnter number of resources in this ACE: ");
            num_resources = scanner.nextInt();
        }

        System.out.println("\nResource properties");
        i = 0;
        while (i < num_resources) {
            OcAceResource res = new OcAceResource(ace);

            if (res == null) {
                System.out.println("\nERROR: Could not allocate new resource for ACE");
                return;
            }

            System.out.print("Have resource href? [0-No, 1-Yes]: ");
            int c = scanner.nextInt();
            if (c == 1) {
                System.out.print("Enter resource href (eg. /a/light): ");
                String href;
                // max string length in C is 64 characters including
                // the nul character, so useable lenght is 63
                href = scanner.next();
                if (href.length() > 63) {
                    href = href.substring(0, 63);
                }

                res.setHref(href);
                res.setWildcard(OCAceWildcard.OC_ACE_NO_WC);
            } else {
                System.out.print("\nSet wildcard resource? [0-No, 1-Yes]: ");
                c = scanner.nextInt();
                if (c == 1) {
                    StringBuilder wildcardMenu = new StringBuilder();
                    wildcardMenu.append("[1]: All NCRs '*'\n");
                    wildcardMenu.append("[2]: All NCRs with >=1   secured endpoint '+'\n");
                    wildcardMenu.append("[3]: All NCRs with >=1 unsecured endpoint '-'\n");
                    wildcardMenu.append("\nSelect wildcard resource: ");
                    System.out.print(wildcardMenu);
                    c = scanner.nextInt();
                    switch (c) {
                    case 1:
                        res.setWildcard(OCAceWildcard.OC_ACE_WC_ALL);
                        break;
                    case 2:
                        res.setWildcard(OCAceWildcard.OC_ACE_WC_ALL_SECURED);
                        break;
                    case 3:
                        res.setWildcard(OCAceWildcard.OC_ACE_WC_ALL_PUBLIC);
                        break;
                    default:
                        break;
                    }
                }
            }
            i++;
        }

        System.out.println("\nSet ACE2 permissions");
        System.out.print("CREATE [0-No, 1-Yes]: ");
        int c = scanner.nextInt();
        if (c == 1) {
            ace.addPermission(OCAcePermissionsMask.CREATE);
        }
        System.out.print("RETRIEVE [0-No, 1-Yes]: ");
        c = scanner.nextInt();
        if (c == 1) {
            ace.addPermission(OCAcePermissionsMask.RETRIEVE);
        }
        System.out.print("UPDATE [0-No, 1-Yes]: ");
        c = scanner.nextInt();
        if (c == 1) {
            ace.addPermission(OCAcePermissionsMask.UPDATE);
        }
        System.out.print("DELETE [0-No, 1-Yes]: ");
        c = scanner.nextInt();
        if (c == 1) {
            ace.addPermission(OCAcePermissionsMask.DELETE);
        }
        System.out.print("NOTIFY [0-No, 1-Yes]: ");
        c = scanner.nextInt();
        if (c == 1) {
            ace.addPermission(OCAcePermissionsMask.NOTIFY);
        }

        int ret = obt.provisionAce(ods[dev].getUuid(), ace, provisionAce2Handler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to provision ACE");
        } else {
            System.out.println("\nERROR issuing request to provision ACE");
        }
    }

    public static void provisionAuthWildcardAce() {
        if (ownedDevices.isEmpty()) {
            System.out.println("\n\nPlease Re-Discover Owned devices");
            return;
        }

        int i = 0;

        StringBuilder ownedDevicesMenu = new StringBuilder();
        ownedDevicesMenu.append("\nProvision auth crypt * ACE\n");
        ownedDevicesMenu.append("My Devices:\n");
        OcfDeviceInfo[] ods = ownedDevices.toArray(new OcfDeviceInfo[ownedDevices.size()]);
        for (OcfDeviceInfo od : ods) {
            ownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(od.getUuid()) + " - " + od.getName() + "\n");
            i++;
        }
        ownedDevicesMenu.append("\nSelect device for provisioning: ");
        System.out.print(ownedDevicesMenu);

        int userInput = scanner.nextInt();
        if (userInput < 0 || userInput >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        int ret = obt.provisionAuthWildcardAce(ods[userInput].getUuid(), provisionAuthWildcardAceHandler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to provision auth-crypt * ACE");
        } else {
            System.out.println("\nERROR issuing request to provision auth-crypt * ACE");
        }
    }

    public static void provisionRoleWildcardAce() {
        if (ownedDevices.isEmpty()) {
            System.out.println("\n\nPlease Re-Discover Owned devices");
            return;
        }

        int i = 0;

        StringBuilder ownedDevicesMenu = new StringBuilder();
        ownedDevicesMenu.append("\nProvision role * ACE\n");
        ownedDevicesMenu.append("My Devices:\n");
        OcfDeviceInfo[] ods = ownedDevices.toArray(new OcfDeviceInfo[ownedDevices.size()]);
        for (OcfDeviceInfo od : ods) {
            ownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(od.getUuid()) + " - " + od.getName() + "\n");
            i++;
        }
        ownedDevicesMenu.append("\nSelect device for provisioning: ");
        System.out.print(ownedDevicesMenu);

        int userInput = scanner.nextInt();
        if (userInput < 0 || userInput >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        System.out.println("\nEnter role: ");
        String role = scanner.next();
        // max string length for role is 64 characters
        if (role.length() > 64) {
            role = role.substring(0, 64);
        }
        String authority = null;
        System.out.print("Authority? [0-No, 1-Yes]: ");
        int c = scanner.nextInt();
        if (c == 1) {
            System.out.println("\nEnter authority: ");
            authority = scanner.next();
            // max string length for role is 64 characters
            if (authority.length() > 64) {
                authority = authority.substring(0, 64);
            }
        }

        int ret = obt.provisionRoleWildcardAce(ods[userInput].getUuid(), role, authority,
                provisionRoleWildcardAceHandler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to provision role * ACE");
        } else {
            System.out.println("\nERROR issuing request to provision role * ACE");
        }
    }

    public static void provisionIdCertificate() {
        if (ownedDevices.isEmpty()) {
            System.out.println("\n\nPlease Re-Discover Owned devices");
            return;
        }

        int i = 0;

        StringBuilder ownedDevicesMenu = new StringBuilder();
        ownedDevicesMenu.append("My Devices:\n");
        OcfDeviceInfo[] ods = ownedDevices.toArray(new OcfDeviceInfo[ownedDevices.size()]);
        for (OcfDeviceInfo od : ods) {
            ownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(od.getUuid()) + " - " + od.getName() + "\n");
            i++;
        }
        ownedDevicesMenu.append("\nSelect device: ");
        System.out.print(ownedDevicesMenu);

        int userInput = scanner.nextInt();
        if (userInput < 0 || userInput >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        int ret = obt.provisionIdentityCertificate(ods[userInput].getUuid(), provisionIdCertificateHandler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to provision identity certificate");
        } else {
            System.out.println("\nERROR issuing request to provision identity certificate");
        }
    }

    public static void provisionRoleCertificate() {
        if (ownedDevices.isEmpty()) {
            System.out.println("\n\nPlease Re-Discover Owned devices");
            return;
        }

        int i = 0;

        StringBuilder ownedDevicesMenu = new StringBuilder();
        ownedDevicesMenu.append("\nProvision role * ACE\n");
        ownedDevicesMenu.append("My Devices:\n");
        OcfDeviceInfo[] ods = ownedDevices.toArray(new OcfDeviceInfo[ownedDevices.size()]);
        for (OcfDeviceInfo od : ods) {
            ownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(od.getUuid()) + " - " + od.getName() + "\n");
            i++;
        }
        ownedDevicesMenu.append("\nSelect device for provisioning: ");
        System.out.print(ownedDevicesMenu);

        int userInput = scanner.nextInt();
        if (userInput < 0 || userInput >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        OCRole roles = null;
        int c;
        do {
            System.out.println("\nEnter role: ");
            String role = scanner.next();
            // max string length for role is 64 characters
            if (role.length() > 64) {
                role = role.substring(0, 64);
            }
            String authority = null;
            System.out.print("Authority? [0-No, 1-Yes]: ");
            c = scanner.nextInt();
            if (c == 1) {
                System.out.println("\nEnter authority: ");
                authority = scanner.next();
                // max string length for role is 64 characters
                if (authority.length() > 64) {
                    authority = authority.substring(0, 64);
                }
            }
            roles = obt.addRoleId(roles, role, authority);
            System.out.print("\nMore Roles? [0-No, 1-Yes]: ");
            c = scanner.nextInt();
        } while (c == 1);

        int ret = obt.provisionRoleCertificate(roles, ods[userInput].getUuid(), provisionRoleCertificateHandler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to provision role certificate");
        } else {
            System.out.println("\nERROR issuing request to provision role certificate");
        }
    }

    private static void resetDevice() {
        if (ownedDevices.isEmpty()) {
            System.out.println("\n\nPlease Re-Discover Owned devices");
            return;
        }

        int i = 0;

        StringBuilder ownedDevicesMenu = new StringBuilder();
        ownedDevicesMenu.append("\nMy Devices:\n");
        OcfDeviceInfo[] ods = ownedDevices.toArray(new OcfDeviceInfo[ownedDevices.size()]);
        for (OcfDeviceInfo od : ods) {
            ownedDevicesMenu
                    .append("[" + i + "]: " + OCUuidUtil.uuidToString(od.getUuid()) + " - " + od.getName() + "\n");
            i++;
        }
        ownedDevicesMenu.append("\nSelect device : ");
        System.out.print(ownedDevicesMenu);

        int userInput = scanner.nextInt();
        if (userInput < 0 || userInput >= i) {
            System.out.println("ERROR: Invalid selection");
            return;
        }

        int ret = obt.deviceHardReset(ods[userInput].getUuid(), resetDeviceHandler);
        if (ret >= 0) {
            System.out.println("\nSuccessfully issued request to perform hard RESET");
        } else {
            System.out.println("\nERROR issuing request to perform hard RESET");
        }
    }

    public static void resetOBT() {
        obtPlatform.reset();
        obt.shutdown();
        ownedDevices.clear();
        unownedDevices.clear();
        obt = new OcObt();
    }

    public static void main(String[] args) {
        Runtime.getRuntime().addShutdownHook(shutdownHook);

        String creds_path = "./onboarding_tool_creds/";
        java.io.File directory = new java.io.File(creds_path);
        if (!directory.exists()) {
            directory.mkdir();
        }
        System.out.println("Storage Config PATH : " + directory.getPath());
        if (0 != OCStorage.storageConfig(directory.getPath())) {
            System.err.println("Failed to setup Storage Config.");
        }

        // Note: If using a factory presets handler,
        // the factory presets handler must be set prior to calling
        // systemInit().
        // The systemInit() function will cause the factory presets handler to
        // be called if it is set.
        OcUtils.setFactoryPresetsHandler(new FactoryPresetsHandler());

        ObtInitHandler obtHandler = new ObtInitHandler(obtPlatform);
        obtPlatform.systemInit(obtHandler);
        obt = new OcObt();

        while (!quit) {
            displayMenu();
            int userInput = 0;
            try {
                userInput = scanner.nextInt();
            } catch (InputMismatchException e) {
                System.out.println("Invalid Input.");
                userInput = 0;
            }
            switch (userInput) {
            case 0:
                continue;
            case 1:
                discoverUnownedDevices(0);
                break;
            case 2:
                discoverUnownedDevices(1);
                break;
            case 3:
                discoverUnownedDevices(2);
                break;
            case 4:
                discoverOwnedDevices(0);
                break;
            case 5:
                discoverOwnedDevices(1);
                break;
            case 6:
                discoverOwnedDevices(2);
                break;
            case 7:
                otmJustWorks();
                break;
            case 8:
                requestRandomPin();
                break;
            case 9:
                otmRandomPin();
                break;
            case 10:
                otmCertificate();
                break;
            case 11:
                provisionCredentials();
                break;
            case 12:
                provisionAce2();
                break;
            case 13:
                provisionAuthWildcardAce();
                break;
            case 14:
                provisionRoleWildcardAce();
                break;
            case 15:
                provisionIdCertificate();
                break;
            case 16:
                provisionRoleCertificate();
                break;
            case 97:
                resetDevice();
                break;
            case 98:
                resetOBT();
                break;
            case 99:
                quit = true;
                break;
            default:
                break;
            }
        }

        obtPlatform.systemShutdown();
        obt.shutdown();
        System.exit(0);
    }
}
