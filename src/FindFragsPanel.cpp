/*
Copyright 2010-2011 Ed Bow <edxbow@gmail.com>

This file is part of Quake Live - Demo Tools (QLDT).

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>
*/

#include "FindFragsPanel.h"
#include "Data.h"
#include "QuakeEnums.h"
#include "ScanWidget.h"
#include "MainTabWidget.h"
#include "DemoTable.h"
#include "MainTable.h"
#include "FindFragsTable.h"
#include "Demo.h"
#include "ProgressDialog.h"
#include "Task.h"

#include <QApplication>
#include <QButtonGroup>
#include <QRadioButton>
#include <QTime>
#include <QLabel>
#include <QPushButton>
#include <QComboBox>
#include <QLineEdit>
#include <QMutex>
#include <QGroupBox>
#include <QGridLayout>
#include <QCheckBox>

using namespace dtdata;

DtFragsTab::DtFragsTab( QWidget* parent ) : QWidget( parent ) {
    sSelector = new DtSourceFilesSelector( this );

    QHBoxLayout* hBox = new QHBoxLayout;

    hBox->addWidget( sSelector, 1 );
    hBox->setSpacing( 10 );

    cbWeapon = new QComboBox( this );
    QLabel* lblWeapon = new QLabel( tr( "Weapon" ), this );

    cbGameType = new QComboBox( this );
    QLabel* lblGameType = new QLabel( tr( "Game Type" ), this );

    leMap = new QLineEdit( this );
    leMap->setMaxLength( 100 );
    leMap->setText( config.frags.leMap );
    cbMap = new QComboBox( this );
    QLabel* lblMap = new QLabel( tr( "Map" ), this );

    directHitsOnlyCb = new QCheckBox( this );
    directHitsOnlyCb->setChecked( config.frags.directHitsOnly );
    QLabel* lblDirectHits = new QLabel( tr( "Direct hits only" ), this );

    leMaxTime = new QLineEdit( this );
    leMaxTime->setValidator( new QIntValidator( this ) );
    leMaxTime->setMaxLength( 4 );
    leMaxTime->setText( config.frags.leMaxTime );
    QLabel* lblMaxTime = new QLabel( tr( "Max. time between frags" ), this );

    leMinFrags = new QLineEdit( this );
    leMinFrags->setValidator( new QIntValidator( this ) );
    leMinFrags->setMaxLength( 2 );
    leMinFrags->setText( config.frags.leMinFrags );
    QLabel* lblMinFrags = new QLabel( tr( "Min. frags" ), this );

    countTeamKillsCb = new QCheckBox( this );
    countTeamKillsCb->setChecked( config.frags.countTeamKills );
    QLabel* lblcountTeamKills = new QLabel( tr( "Team kills" ), this );

    sSelector->textFields.at( 0 )->setText( config.frags.playerName );
    sSelector->textFields.at( 1 )->setText( config.frags.playerNames );

    startScanButton = new QPushButton( tr( "Find" ), this );
    connect( startScanButton, SIGNAL( clicked() ), this, SLOT( startScan() ) );

    closeButton = new QPushButton( tr( "Close" ), this );
    connect( closeButton, SIGNAL( clicked() ), parent, SLOT( hideFindFragsWidget() ) );

    QGridLayout* grid = new QGridLayout;

    grid->addWidget( lblWeapon, 0, 0 );
    grid->addWidget( cbWeapon, 0, 1 );
    grid->addWidget( lblGameType, 1, 0 );
    grid->addWidget( cbGameType, 1, 1 );
    grid->addWidget( lblMap, 2, 0 );

    grid->addWidget( cbMap, 2, 1 );
    grid->addWidget( leMap, 3, 1 );
    grid->setColumnMinimumWidth( 2, 20 );
    grid->addWidget( lblDirectHits, 0, 3 );
    grid->addWidget( directHitsOnlyCb, 0, 4 );
    grid->addWidget( lblMaxTime, 1, 3 );
    grid->addWidget( leMaxTime, 1, 4 );
    grid->addWidget( lblMinFrags, 2, 3 );
    grid->addWidget( leMinFrags, 2, 4 );
    grid->addWidget( lblcountTeamKills, 3, 3 );
    grid->addWidget( countTeamKillsCb, 3, 4 );
    grid->addWidget( startScanButton, 4, 3 );
    grid->addWidget( closeButton, 4, 4 );

    hBox->addLayout( grid, 1 );
    hBox->addSpacing( 10 );

    QGroupBox* mainGroup = new QGroupBox;
    mainGroup->setLayout( hBox );

    QHBoxLayout* mainLayout = new QHBoxLayout;
    mainLayout->addWidget( mainGroup );

    setLayout( mainLayout );

    initNames();

    cbWeapon->setCurrentIndex( config.frags.cbWeapon );
    cbGameType->setCurrentIndex( config.frags.cbGameType );
    cbMap->setCurrentIndex( config.frags.cbMap );

    task = new DtTask;
    scanMutex = new QMutex;

    progressDialog = new DtProgressDialog( tr( "Searching" ), this );
    connect( progressDialog, SIGNAL( buttonClicked() ), task, SLOT( stop() ) );

    connect( this, SIGNAL( newScanStarted() ), this, SLOT( updateProgress() ) );
    connect( this, SIGNAL( demoGamestateParsed( int ) ), mainDemoTable, SLOT( updateRow( int ) ) );
}

DtFragsTab::~DtFragsTab() {
    saveSettings();

    delete task;
    delete scanMutex;
}

void DtFragsTab::updateProgress() {
    QString lbl = QString::number( task->finishedJobsNum() ) + " " + tr( "of" ) + " "
                  + QString::number( task->jobCount() );
    int prc = static_cast< int >( doneMb / demosSize * 100 );
    progressDialog->setData( lbl, prc, DtProgressDialog::CancelButton );
}

void DtFragsTab::addResult( DtDemo* demo, int startSeqTime, int fragsInSeq, int deltaTime,
                            int segLength, int weapon )
{
    scanMutex->lock();
    DtFindFragsTable* table = qobject_cast< DtFindFragsTable* >( currentScanWidget->scanTable );
    table->addScanRow( demo, startSeqTime, fragsInSeq, deltaTime, segLength, weapon );
    scanMutex->unlock();
    QApplication::processEvents();
    haveResults = true;
}

bool DtFragsTab::weaponMatch( int wp, int mod ) {
    if ( mod == MOD_GRENADE_SPLASH   ||
         mod == MOD_ROCKET_SPLASH    ||
         mod == MOD_PLASMA_SPLASH    ||
         mod == MOD_BFG_SPLASH )
    {
        if ( directHitsOnlyCb->isChecked() ) {
            return false;
        }

        --mod;
    }

    return ( wp == -1 || mod == wp );
}

void DtFragsTab::scanDemo( int index ) {
    DtDemo* demo = currentFilteredDemos.at( index );

    bool parseDone = demo->findFrags();

    scanMutex->lock();
    doneMb += demo->fileInfo().size / MiB;
    scanMutex->unlock();

    emit newScanStarted();

    if ( parseDone ) {
        int lastFragTime = -1;
        int startSeqTime = 0;
        int fragsInSeq = 0;
        int deltaTime = 0;
        bool countTeamKills = countTeamKillsCb->isChecked();

        QMapIterator< int, int > it( demo->getObitEvents() );

        while ( it.hasNext() ) {
            it.next();
            const int fragTime = it.key();
            int meanOfDeath = it.value();

            if ( fragTime <= demo->getMapRestartTime() ) {
                continue; /* exclude warmup frags */
            }

            if ( lastFragTime == -1 ) {
                lastFragTime = fragTime;
            }

            bool tk = false;

            if ( meanOfDeath < -1 ) { /* teamkill */
                meanOfDeath += 50;

                if ( demo->getGameType() != GT_FFA ) { /* in ffa all players belong to the one team */
                    tk = true;
                }
            }

            bool weaponMatches = weaponMatch( weapon, meanOfDeath );

            if ( meanOfDeath == -1                      ||   /* player death or respawn */
                 ( !countTeamKills && tk )              ||   /* don't count teamkill */
                 ( fragTime - lastFragTime > maxTime )  ||   /* time between adjacent frags is greater
                                                                than specified */
                 !weaponMatches )
            {
                if ( fragsInSeq >= minFrags ) {
                    addResult( demo, startSeqTime, fragsInSeq, deltaTime,
                               lastFragTime - startSeqTime, weapon );
                }

                lastFragTime = -1;
                fragsInSeq = 0;

                if ( minFrags != 1 ) {
                    continue;
                }
            }

            if ( weaponMatches && fragsInSeq == 0 ) {
                startSeqTime = fragTime;
                deltaTime = 0;
            }

            if ( weaponMatches && meanOfDeath != -1 &&
                 ( ( !tk && !countTeamKills ) || countTeamKills ) )
            {
                ++fragsInSeq;
            }

            int delta = fragTime - lastFragTime;

            if ( deltaTime < delta && minFrags != 1 ) {
                deltaTime = delta;
            }

            lastFragTime = fragTime;
        }

        if ( fragsInSeq >= minFrags ) {
            addResult( demo, startSeqTime, fragsInSeq, deltaTime,
                       lastFragTime - startSeqTime, weapon );
        }
    }

    scanMutex->lock();
    task->setJobDone();
    scanMutex->unlock();
}

void DtFragsTab::startScan() {
    currentScanWidget = mainTabWidget->addScanWidget( DtTablesWidget::TT_FINDFRAGS );

    startScanButton->setEnabled( false );
    closeButton->setEnabled( false );
    progressDialog->start();
    progressDialog->show();
    QApplication::processEvents();

    int filterIndex = sSelector->selectedIndex();

    currentFilteredDemos.clear();
    DtDemoVec currentSelectedDemos;

    if ( filterIndex == F_SELECTED ) {
         currentSelectedDemos = mainDemoTable->getSelectedDemos();
    }
    else {
        DtTableHashIterator it( mainDemoTable->getTableHash() );

        while ( it.hasNext() ) {
            it.next();
            currentSelectedDemos.append( mainDemoTable->demoAt( it.key()->row() ) );
        }
    }

    minFrags = leMinFrags->text().toInt();

    if ( minFrags <= 0 ) {
        minFrags = 1;
    }

    maxTime = leMaxTime->text().toInt();

    if ( maxTime <= 0 ) {
        maxTime = 1;
    }

    weapon = cbWeapon->itemData( cbWeapon->currentIndex() ).toInt();
    gameType = cbGameType->itemData( cbGameType->currentIndex() ).toInt();
    QString map = cbMap->itemText( cbMap->currentIndex() );

    if ( cbMap->currentIndex() == 1 ) { /* Other */
        if ( !leMap->text().isEmpty() ) {
            map = leMap->text();
        }
        else {
            cbMap->setCurrentIndex( 0 );
        }
    }

    QString searchInfo;
    searchInfo = tr( "weapon: %1%2"
                     "        "
                     "game type: %3"
                     "        "
                     "map: %4"
                     "        "
                     "max time: %5"
                     "        "
                     "min frags: %6" );

    QString directHits = "";

    if ( config.frags.directHitsOnly ) {
        directHits = " (" + tr( "direct hits" ) + ")";
    }

    currentScanWidget->setSearchInfo(
            searchInfo.arg( cbWeapon->itemText( cbWeapon->currentIndex() ),
                            directHits,
                            cbGameType->itemText( cbGameType->currentIndex() ),
                            cbMap->itemText( cbMap->currentIndex() ) )
                      .arg( maxTime )
                      .arg( minFrags ) );

    maxTime *= 1000;
    haveResults = false;
    demosSize = 0.f;
    doneMb = 0.f;

    QDate startWeek;
    QDate date = QDate::currentDate();
    QDate startMonth( date.year(), date.month(), 1 );
    QStringList playerNames;
    int playerNamesCount = 0;

    if ( filterIndex == F_THIS_WEEK ) {
        int year = date.year();
        int month = date.month();
        int day = date.day();

        if ( date.dayOfWeek() > date.day() ) {
            if ( month == 1 ) {
                month = 12;
                --year;
            }
            else {
                --month;
            }

            startWeek.setDate( year, month, 1 );
            day = startWeek.daysInMonth() - date.dayOfWeek() + date.day() + 1;
        }
        else if ( date.dayOfWeek() < date.day() ) {
            day -= date.dayOfWeek() - 1;
        }

        startWeek.setDate( year, month, day );
    }
    else if ( filterIndex == F_PLAYERS ) {
        playerNames = sSelector->textFields.at( 1 )->text().split( ',', QString::SkipEmptyParts );
        playerNamesCount = playerNames.count();
    }

    for ( int i = 0; i < currentSelectedDemos.size(); ++i ) {
        DtDemo* demo = currentSelectedDemos.at( i );

        if ( !demo->isGamesateParsed() ) {
            if ( !demo->parseGamestateMsg() ) {
                continue;
            }
            else {
                QTableWidgetItem* demoItem = mainDemoTable->getTableHash().key( demo, 0 );

                if ( demoItem ) {
                    emit demoGamestateParsed( demoItem->row() );
                }
            }
        }

        if ( filterIndex >= F_TODAY && filterIndex <= F_THIS_MONTH ) {
            QDateTime demoTime;
            demoTime.setTime_t( demo->getLevelStartTime() );
            QDate demoDate = demoTime.date();

            if ( filterIndex == F_TODAY ) {
                if ( date != demoDate ) {
                    continue;
                }
            }
            else if ( filterIndex == F_THIS_WEEK ) {
                if ( date.dayOfWeek() == date.day() && date != demoDate ) {
                    continue;
                }
                else if ( demoDate < startWeek ) {
                    continue;
                }
            }
            else if ( filterIndex == F_THIS_MONTH && demoDate < startMonth ) {
                continue;
            }
        }
        else if ( filterIndex == F_RECOREDED_BY &&
                  demo->getClientName() != sSelector->textFields.at( 0 )->text() )
        {
            continue;
        }
        else if ( filterIndex == F_PLAYERS ) {
            if ( !playerNames.isEmpty() ) {
                const infoMap& info = demo->getInfo();
                int infoCount = info.count();
                bool havePlayer = false;

                for ( int i = 0; i < infoCount; ++i ) {
                    if ( info.at( i ).first.startsWith( "playerName" ) ) {
                        for ( int j = 0; j < playerNamesCount; ++j ) {
                            if ( info.at( i ).second == playerNames.at( j ).trimmed() ) {
                                havePlayer = true;
                                break;
                            }
                        }

                        if ( havePlayer ) {
                            break;
                        }
                    }
                }

                if ( !havePlayer ) {
                    continue;
                }
            }
            else {
                continue;
            }
        }

        if ( gameType != GT_ANY ) {
            int compareGameType = gameType;

            if ( demo->getProto() == Q3_68 ) {
                if ( gameType == GT_CTF ) {
                    compareGameType = ( demo->getQ3Mod() == MOD_CPMA ) ?
                                      static_cast< int >( GT_CTF_CPMA ) : GT_CTF_Q3;
                }
                else if ( gameType == GT_CA ) {
                    compareGameType = GT_CA_CPMA;
                }
            }

            if ( demo->getGameType() != gameType ) {
                continue;
            }
        }

        if ( cbMap->currentIndex() != 0 && map.toLower() != demo->getMapName() ) {
            continue;
        }

        currentFilteredDemos.append( demo );
        demosSize += demo->fileInfo().size / MiB;
    }

    int demosCount = currentFilteredDemos.size();

    progressDialog->setData( "0 " + tr( "of" ) + " " + QString::number( demosCount ), 0,
                             DtProgressDialog::CancelButton );

    task->run( demosCount, this, &DtFragsTab::scanDemo, &DtFragsTab::scanWaitFunc );
    qApp->processEvents();

    if ( haveResults ) {
        progressDialog->close();
    } else {
        progressDialog->setData( tr( "Nothing found" ), 100, DtProgressDialog::OkButton );
    }

    emit scanFinished();
    currentScanWidget->scanTable->sortColumn(
            config.findFragsTableSortColumn,
            static_cast< Qt::SortOrder >( config.findFragsTableSortOrder ) );
    startScanButton->setEnabled( true );
    closeButton->setEnabled( true );
}

void DtFragsTab::scanWaitFunc() {
    QApplication::processEvents();
}

void DtFragsTab::saveSettings() {
    config.frags.selectorIndex = sSelector->selectedIndex() == 1 ? 0 : sSelector->selectedIndex();
    config.frags.cbWeapon = cbWeapon->currentIndex();
    config.frags.cbGameType = cbGameType->currentIndex();
    config.frags.cbMap = cbMap->currentIndex();
    config.frags.leMap = leMap->text();
    config.frags.leMaxTime = leMaxTime->text();
    config.frags.leMinFrags = leMinFrags->text();
    config.frags.playerName = sSelector->textFields.at( 0 )->text();
    config.frags.playerNames = sSelector->textFields.at( 1 )->text();
    config.frags.directHitsOnly = directHitsOnlyCb->isChecked();
    config.frags.countTeamKills = countTeamKillsCb->isChecked();
}

DtSourceFilesSelector::DtSourceFilesSelector( QWidget* parent ) : QWidget( parent ) {
    QStringList sourceNames;

    sourceNames << tr( "All" ) << tr( "Selected" ) << tr( "Today" ) << tr( "This week" );
    sourceNames << tr( "This month" ) << tr( "Recorded by" ) << tr( "Have players" );

    textFieldNums.append( 5 );
    textFieldNums.append( 6 );

    sourceFilesGroup = new QButtonGroup( this );
    connect( sourceFilesGroup, SIGNAL( buttonPressed( int ) ),
             this, SLOT( selectionChanged( int ) ) );

    sourceFilesGBox = new QGroupBox( tr( "Files" ), this );
    sourceFilesGBox->setMinimumWidth( 300 );

    gBox = new QGridLayout;

    for ( int i = 0; i < sourceNames.size(); ++i ) {
        QRadioButton* sButton = new QRadioButton( sourceNames.at( i ) );
        sourceFilesGroup->addButton( sButton, i );
        gBox->addWidget( sButton, i, 0 );

        if ( !i ) {
            sButton->setChecked( true );
        }

        QListIterator< int > it( textFieldNums );

        while ( it.hasNext() ) {
            if ( it.next() == i ) {
                QLineEdit* tF = new QLineEdit( this );

                tF->setVisible( false );
                tF->setMinimumHeight( 25 );
                gBox->addWidget( tF, i, 1 );
                textFields.append( tF );
            }
        }
    }

    sourceFilesGBox->setLayout( gBox );
}

void DtSourceFilesSelector::selectionChanged( int id ) {
    QListIterator< int > it( textFieldNums );
    int inList = -1;

    while ( it.hasNext() ) {
        if ( it.next() == id ) {
            gBox->itemAtPosition( id, 1 )->widget()->setVisible( true );
            inList = id;
        }
    }

    it.toFront();

    while ( it.hasNext() ) {
        int pos = it.next();
        if ( inList != pos ) {
            gBox->itemAtPosition( pos, 1 )->widget()->setVisible( false );
        }
    }
}

int DtSourceFilesSelector::selectedIndex() {
    return sourceFilesGroup->checkedId();
}

void DtFragsTab::initNames() {
    #define WEA( x ) cbWeapon->addItem( getWeaponName( x ), x )

    WEA( -1 );
    WEA( MOD_GAUNTLET );
    WEA( MOD_MACHINEGUN );
    WEA( MOD_SHOTGUN );
    WEA( MOD_GRENADE );
    WEA( MOD_ROCKET );
    WEA( MOD_RAILGUN );
    WEA( MOD_PLASMA );
    WEA( MOD_LIGHTNING );
    WEA( MOD_BFG );
    WEA( MOD_CHAINGUN );
    WEA( MOD_NAIL );
    WEA( MOD_PROXIMITY_MINE );
    WEA( MOD_KAMIKAZE );
    WEA( MOD_TELEFRAG );

    #define GT( x ) cbGameType->addItem( getGameTypeName( QZ_73, 0, x ), x )

    GT( GT_ANY );
    GT( GT_FFA );
    GT( GT_DUEL );
    GT( GT_TDM );
    GT( GT_CA );
    GT( GT_CTF );
    GT( GT_1FCTF );
    GT( GT_OVERLOAD );
    GT( GT_HARVESTER );
    GT( GT_FREEZE );
    GT( GT_DOM );
    GT( GT_AD );
    GT( GT_RR );
    GT( GT_RACE );
    GT( GT_INSTAGIB );
    GT( GT_INSTACTF );
    GT( GT_INSTATDM );

    cbMap->addItem( tr( "Any" ) );
    cbMap->addItem( tr( "Other (type below)" ) );
    cbMap->addItem( "aerowalk" );
    cbMap->addItem( "almostlost" );
    cbMap->addItem( "arcanecitadel" );
    cbMap->addItem( "arenagate" );
    cbMap->addItem( "asylum" );
    cbMap->addItem( "basesiege" );
    cbMap->addItem( "battleforged" );
    cbMap->addItem( "beyondreality" );
    cbMap->addItem( "blackcathedral" );
    cbMap->addItem( "bloodlust" );
    cbMap->addItem( "bloodrun" );
    cbMap->addItem( "brimstoneabbey" );
    cbMap->addItem( "campercrossings" );
    cbMap->addItem( "campgrounds" );
    cbMap->addItem( "cannedheat" );
    cbMap->addItem( "chemicalreaction" );
    cbMap->addItem( "cliffside" );
    cbMap->addItem( "cobaltstation" );
    cbMap->addItem( "coldcathode" );
    cbMap->addItem( "coldwar" );
    cbMap->addItem( "concretepalace" );
    cbMap->addItem( "corrosion" );
    cbMap->addItem( "courtyard" );
    cbMap->addItem( "cure" );
    cbMap->addItem( "deepinside" );
    cbMap->addItem( "delirium" );
    cbMap->addItem( "demonkeep" );
    cbMap->addItem( "devilish" );
    cbMap->addItem( "diesirae" );
    cbMap->addItem( "dismemberment" );
    cbMap->addItem( "distantscreams" );
    cbMap->addItem( "doubleimpact" );
    cbMap->addItem( "dreadfulplace" );
    cbMap->addItem( "dredwerkz" );
    cbMap->addItem( "duelingkeeps" );
    cbMap->addItem( "electrichead" );
    cbMap->addItem( "eviscerated" );
    cbMap->addItem( "evolution" );
    cbMap->addItem( "eyetoeye" );
    cbMap->addItem( "falloutbunker" );
    cbMap->addItem( "fatalinstinct" );
    cbMap->addItem( "finnegans" );
    cbMap->addItem( "fluorescent" );
    cbMap->addItem( "focalpoint" );
    cbMap->addItem( "foolishlegacy" );
    cbMap->addItem( "forgotten" );
    cbMap->addItem( "furiousheights" );
    cbMap->addItem( "fuse" );
    cbMap->addItem( "golgothacore" );
    cbMap->addItem( "gothicrage" );
    cbMap->addItem( "grimdungeons" );
    cbMap->addItem( "hearth" );
    cbMap->addItem( "hektik" );
    cbMap->addItem( "hellsgate" );
    cbMap->addItem( "hellsgateredux" );
    cbMap->addItem( "heroskeep" );
    cbMap->addItem( "hiddenfortress" );
    cbMap->addItem( "houseofdecay" );
    cbMap->addItem( "industrialaccident" );
    cbMap->addItem( "infinity" );
    cbMap->addItem( "innersanctums" );
    cbMap->addItem( "intervention" );
    cbMap->addItem( "ironworks" );
    cbMap->addItem( "japanesecastles" );
    cbMap->addItem( "jumpwerkz" );
    cbMap->addItem( "leftbehind" );
    cbMap->addItem( "leviathan" );
    cbMap->addItem( "limbus" );
    cbMap->addItem( "longestyard" );
    cbMap->addItem( "lostworld" );
    cbMap->addItem( "namelessplace" );
    cbMap->addItem( "overkill" );
    cbMap->addItem( "overlord" );
    cbMap->addItem( "phrantic" );
    cbMap->addItem( "pillbox" );
    cbMap->addItem( "powerstation" );
    cbMap->addItem( "provinggrounds" );
    cbMap->addItem( "purgatory" );
    cbMap->addItem( "quarantine" );
    cbMap->addItem( "qzpractice1" );
    cbMap->addItem( "qzpractice2" );
    cbMap->addItem( "realmofsteelrats" );
    cbMap->addItem( "rebound" );
    cbMap->addItem( "reflux" );
    cbMap->addItem( "repent" );
    cbMap->addItem( "retribution" );
    cbMap->addItem( "revolver" );
    cbMap->addItem( "sacellum" );
    cbMap->addItem( "scornforge" );
    cbMap->addItem( "seamsandbolts" );
    cbMap->addItem( "shiningforces" );
    cbMap->addItem( "siberia" );
    cbMap->addItem( "silence" );
    cbMap->addItem( "silentfright" );
    cbMap->addItem( "sinister" );
    cbMap->addItem( "skyward" );
    cbMap->addItem( "solid" );
    cbMap->addItem( "somewhatdamaged" );
    cbMap->addItem( "sorrow" );
    cbMap->addItem( "spacecamp" );
    cbMap->addItem( "spacechamber" );
    cbMap->addItem( "spacectf" );
    cbMap->addItem( "spidercrossings" );
    cbMap->addItem( "spillway" );
    cbMap->addItem( "stonekeep" );
    cbMap->addItem( "stronghold" );
    cbMap->addItem( "terminalheights" );
    cbMap->addItem( "terminatria" );
    cbMap->addItem( "terminus" );
    cbMap->addItem( "theatreofpain" );
    cbMap->addItem( "theedge" );
    cbMap->addItem( "threestory" );
    cbMap->addItem( "thunderstruck" );
    cbMap->addItem( "tornado" );
    cbMap->addItem( "toxicity" );
    cbMap->addItem( "trinity" );
    cbMap->addItem( "troubledwaters" );
    cbMap->addItem( "useandabuse" );
    cbMap->addItem( "verticalvengeance" );
    cbMap->addItem( "vortexportal" );
    cbMap->addItem( "warehouse" );
    cbMap->addItem( "wargrounds" );
    cbMap->addItem( "wicked" );
    cbMap->addItem( "windowpain" );
    cbMap->addItem( "windsongkeep" );
}
