// Copyright 2014 Stellar Development Foundation and contributors. Licensed
// under the ISC License. See the COPYING file at the top-level directory of
// this distribution or at http://opensource.org/licenses/ISC

#include "AccountEntry.h"
#include "LedgerMaster.h"

namespace stellar
{
    AccountEntry::AccountEntry(const stellarxdr::LedgerEntry& from) : LedgerEntry(from)
    {

    }

    void AccountEntry::storeDelete(Json::Value& txResult, LedgerMaster& ledgerMaster)
    {
        std::string base58ID;
        toBase58(getIndex(), base58ID);

        txResult["effects"]["delete"][base58ID];

        ledgerMaster.getDatabase().getSession() << "DELETE from Accounts where accountID=" << base58ID;
    }

    void AccountEntry::storeChange(LedgerEntry::pointer startFrom, Json::Value& txResult, LedgerMaster& ledgerMaster)
    {  
        std::string base58ID;
        toBase58(getIndex(), base58ID);

        std::stringstream sql;
        sql << "UPDATE Accounts set ";

        bool before = false;

        if(mEntry.account().balance != startFrom->mEntry.account().balance)
        { 
            sql << " balance= " << mEntry.account().balance;
            txResult["effects"]["mod"][base58ID]["balance"] = mEntry.account().balance;
            
            before = true;
        }
        
        if(mEntry.account().sequence != startFrom->mEntry.account().sequence)
        {
            if(before) sql << ", ";
            sql << " sequence= " << mEntry.account().sequence;
            txResult["effects"]["mod"][base58ID]["sequence"] = mEntry.account().sequence;
            before = true;
        }

        if(mEntry.account().transferRate != startFrom->mEntry.account().transferRate)
        {
            if(before) sql << ", ";
            sql << " transferRate= " << mEntry.account().transferRate;
            txResult["effects"]["mod"][base58ID]["transferRate"] = mEntry.account().transferRate;
            before = true;
        }

        if(mEntry.account().pubKey != startFrom->mEntry.account().pubKey)
        {
            if(before) sql << ", ";
            sql << " pubKey= " << mEntry.account().pubKey;
            txResult["effects"]["mod"][base58ID]["pubKey"] = mEntry.account().pubKey;
            before = true;
        }

        if(mEntry.account().inflationDest != startFrom->mEntry.account().inflationDest)
        {
            if(before) sql << ", ";
            sql << " inflationDest= " << mEntry.account().inflationDest;
            txResult["effects"]["mod"][base58ID]["inflationDest"] = mEntry.account().inflationDest;
            before = true;
        }

        if(mEntry.account().creditAuthKey != startFrom->mEntry.account().creditAuthKey)
        {
            if(before) sql << ", ";
            sql << " creditAuthKey= " << mEntry.account().creditAuthKey;
            txResult["effects"]["mod"][base58ID]["creditAuthKey"] = mEntry.account().creditAuthKey;
            before = true;
        }

        if(mEntry.account().flags != startFrom->mEntry.account().flags)
        {
            if(before) sql << ", ";
            sql << " flags= " << mEntry.account().flags;
            txResult["effects"]["mod"][base58ID]["flags"] = mEntry.account().flags;
        }

        // TODO.3   KeyValue data
        sql << " where accountID=" << base58ID;
        ledgerMaster.getDatabase().getSession() << sql;
    }


    void AccountEntry::storeAdd(Json::Value& txResult, LedgerMaster& ledgerMaster)
    {
        std::string base58ID;
        toBase58(getIndex(), base58ID);

        ledgerMaster.getDatabase().getSession() << "INSERT into Accounts (accountID,balance) values (:v1,:v2)",
                use(base58ID), use(mEntry.account().balance);

        txResult["effects"]["new"][base58ID]["balance"] = mEntry.account().balance;
    }

    const char *AccountEntry::kSQLCreateStatement = "CREATE TABLE IF NOT EXISTS Accounts (						\
		accountID		CHARACTER(35) PRIMARY KEY,	\
		balance			BIGINT UNSIGNED,			\
		sequence		INT UNSIGNED default 1,				\
		ownerCount		INT UNSIGNED default 0,			\
		transferRate	INT UNSIGNED default 0,		\
        inflationDest	CHARACTER(35),		\
        inflationDest	CHARACTER(35),		\
		inflationDest	CHARACTER(35),		\
		publicKey		CHARACTER(56),		\
		flags		    INT UNSIGNED default 0  	\
	);";

    /* NICOLAS
    

	
	AccountEntry::AccountEntry(SLE::pointer sle)
	{
		mAccountID=sle->getFieldAccount160(sfAccount);
		mBalance = sle->getFieldAmount(sfBalance).getNValue();
		mSequence=sle->getFieldU32(sfSequence);
		mOwnerCount=sle->getFieldU32(sfOwnerCount);
		if(sle->isFieldPresent(sfTransferRate))
			mTransferRate=sle->getFieldU32(sfTransferRate);
		else mTransferRate = 0;

		if(sle->isFieldPresent(sfInflationDest))
			mInflationDest=sle->getFieldAccount160(sfInflationDest);
		
		uint32_t flags = sle->getFlags();

		mRequireDest = flags & lsfRequireDestTag;
		mRequireAuth = flags & lsfRequireAuth;

		// if(sle->isFieldPresent(sfPublicKey)) 
		//	mPubKey=
	}

	void AccountEntry::calculateIndex()
	{
		Serializer  s(22);

		s.add16(spaceAccount); //  2
		s.add160(mAccountID);  // 20

		mIndex= s.getSHA512Half();
	}

	void  AccountEntry::insertIntoDB()
	{
		//make sure it isn't already in DB
		deleteFromDB();

		string sql = str(boost::format("INSERT INTO Accounts (accountID,balance,sequence,owenerCount,transferRate,inflationDest,publicKey,requireDest,requireAuth) values ('%s',%d,%d,%d,%d,'%s','%s',%d,%d);")
			% mAccountID.base58Encode(RippleAddress::VER_ACCOUNT_ID)
			% mBalance
			% mSequence
			% mOwnerCount
			% mTransferRate
			% mInflationDest.base58Encode(RippleAddress::VER_ACCOUNT_ID)
			% mPubKey.base58Key()
			% mRequireDest
			% mRequireAuth);

		Database* db = getApp().getWorkingLedgerDB()->getDB();

		if(!db->executeSQL(sql, true))
		{
			CLOG(WARNING, "Ledger") << "SQL failed: " << sql;
		}
	}
	void AccountEntry::updateInDB()
	{
		string sql = str(boost::format("UPDATE Accounts set balance=%d, sequence=%d,owenerCount=%d,transferRate=%d,inflationDest='%s',publicKey='%s',requireDest=%d,requireAuth=%d where accountID='%s';")
			% mBalance
			% mSequence
			% mOwnerCount
			% mTransferRate
			% mInflationDest.base58Encode(RippleAddress::VER_ACCOUNT_ID)
			% mPubKey.base58Key()
			% mRequireDest
			% mRequireAuth
			% mAccountID.base58Encode(RippleAddress::VER_ACCOUNT_ID));

		Database* db = getApp().getWorkingLedgerDB()->getDB();

		if(!db->executeSQL(sql, true))
		{
			CLOG(WARNING, ripple::Ledger) << "SQL failed: " << sql;
		}
	}
	void AccountEntry::deleteFromDB()
	{
		string sql = str(boost::format("DELETE from Accounts where accountID='%s';")
			% mAccountID.base58Encode(RippleAddress::VER_ACCOUNT_ID));

		Database* db = getApp().getWorkingLedgerDB()->getDB();

		if(!db->executeSQL(sql, true))
		{
			CLOG(WARNING, ripple::Ledger) << "SQL failed: " << sql;
		}
	}

	bool AccountEntry::checkFlag(LedgerSpecificFlags flag)
	{
		// TODO: 
		return(true);
	}

	// will return tesSUCCESS or that this account doesn't have the reserve to do this
	TxResultCode AccountEntry::tryToIncreaseOwnerCount()
	{
		// The reserve required to create the line.
		uint64_t reserveNeeded = gLedgerMaster->getCurrentLedger()->getReserve(mOwnerCount + 1);

		if(mBalance >= reserveNeeded)
		{
			mOwnerCount++;
			return tesSUCCESS;
		}
		return tecINSUF_RESERVE_LINE;
	}

    void AccountEntry::dropAll(Database &db)
    {
        if (!db.getDBCon()->getDB()->executeSQL("DROP TABLE IF EXISTS Accounts;"))
		{
            throw std::runtime_error("Could not drop Account data");
		}
        if (!db.getDBCon()->getDB()->executeSQL(kSQLCreateStatement))
        {
            throw std::runtime_error("Could not recreate Account data");
		}
    }
    */
}

