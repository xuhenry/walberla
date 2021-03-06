//======================================================================================================================
//
//  This file is part of waLBerla. waLBerla is free software: you can 
//  redistribute it and/or modify it under the terms of the GNU General Public
//  License as published by the Free Software Foundation, either version 3 of 
//  the License, or (at your option) any later version.
//  
//  waLBerla is distributed in the hope that it will be useful, but WITHOUT 
//  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or 
//  FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License 
//  for more details.
//  
//  You should have received a copy of the GNU General Public License along
//  with waLBerla (see COPYING.txt). If not, see <http://www.gnu.org/licenses/>.
//
//! \file BoundaryHandlingCollection.h
//! \ingroup boundary
//! \author Florian Schornbaum <florian.schornbaum@fau.de>
//
//======================================================================================================================

#pragma once

#include "Boundary.h"
#include "BoundaryHandling.h"

#include "core/DataTypes.h"
#include "core/cell/CellInterval.h"
#include "core/debug/Debug.h"
#include "core/logging/Logging.h"
#include "core/uid/UID.h"
#include "core/uid/UIDGenerators.h"

#include "domain_decomposition/IBlock.h"

#include "field/FlagField.h"

#include <boost/tuple/tuple.hpp>
#include <ostream>
#include <string>
#include <vector>


namespace walberla {
namespace boundary {



class BHCUIDGenerator : public uid::IndexGenerator< BHCUIDGenerator, uint_t >{};
typedef UID< BHCUIDGenerator > BoundaryHandlingCollectionUID;



template< typename FlagField_T, typename Tuple > // Tuple: all the boundary handlers that are considered by this boundary handling collection
class BoundaryHandlingCollection
{
public:

   typedef FlagField_T                               FlagField;
   typedef typename FlagField_T::flag_t              flag_t;
   typedef typename FlagField_T::const_base_iterator ConstFlagFieldBaseIterator;



   class BlockSweep {
   public:
      BlockSweep( const BlockDataID & collection, const uint_t numberOfGhostLayersToInclude = 0 ) :
         collection_( collection ), numberOfGhostLayersToInclude_( numberOfGhostLayersToInclude ) {}
      void operator()( IBlock * block ) const {
         BoundaryHandlingCollection * collection = block->getData< BoundaryHandlingCollection >( collection_ );
         (*collection)( numberOfGhostLayersToInclude_ );
      }
   protected:
      const BlockDataID collection_;
      const uint_t numberOfGhostLayersToInclude_;
   };



   BoundaryHandlingCollection( const std::string & identifier, FlagField_T * const flagField, const Tuple & boundaryHandlers );

   bool operator==( const BoundaryHandlingCollection &     ) const { WALBERLA_ASSERT( false ); return false; } // For testing purposes, block data items must be comparable with operator "==".
                                                                                                      // Since instances of type "BoundaryHandlingCollection" are registered as block data items,
                                                                                                      // "BoundaryHandlingCollection" must implement operator "==". As of right now, comparing
                                                                                                      // two boundary handling collection instances will always fail... :-) TODO: fixit?
   bool operator!=( const BoundaryHandlingCollection & rhs ) const { return !operator==( rhs ); }

   const BoundaryHandlingCollectionUID & getUID() const { return uid_; }

   const FlagField_T * getFlagField() const { return flagField_; }
         FlagField_T * getFlagField()       { return flagField_; }

   inline bool isEmpty( const cell_idx_t x, const cell_idx_t y, const cell_idx_t z ) const;
   inline bool isEmpty( const ConstFlagFieldBaseIterator & it ) const;

   inline bool consideredByAllHandlers( const uint_t numberOfGhostLayersToInclude = 0 ) const;               // These functions check if each
   inline bool consideredByAllHandlers( const cell_idx_t x, const cell_idx_t y, const cell_idx_t z ) const;  // selected cell is either marked
   inline bool consideredByAllHandlers( const ConstFlagFieldBaseIterator & it ) const;                       // as domain or boundary
   inline bool consideredByAllHandlers( const CellInterval & cells ) const;                                  // in every boundary handling
   template< typename CellIterator >                                                                         // that belongs to this collection.
   inline bool consideredByAllHandlers( const CellIterator & begin, const CellIterator & end ) const;        // <--

   template< typename BoundaryHandling_T >
   inline const BoundaryHandling_T & getBoundaryHandling( const BoundaryHandlingUID & uid ) const; ///< You most likely have to call this function via "collection.template getBoundaryHandling< BoundaryHandling_T >(uid)".
   template< typename BoundaryHandling_T >
   inline       BoundaryHandling_T & getBoundaryHandling( const BoundaryHandlingUID & uid );       ///< You most likely have to call this function via "collection.template getBoundaryHandling< BoundaryHandling_T >(uid)".

   inline uint_t numberOfMatchingHandlers           ( const flag_t flag ) const;
   inline uint_t numberOfMatchingHandlersForDomain  ( const flag_t flag ) const;
   inline uint_t numberOfMatchingHandlersForBoundary( const flag_t flag ) const;

   inline bool containsBoundaryCondition( const BoundaryUID & uid ) const;
   inline bool containsBoundaryCondition( const FlagUID & flag ) const;
   inline bool containsBoundaryCondition( const flag_t flag ) const;

   inline flag_t getBoundaryMask( const BoundaryUID & uid ) const { return getBoundaryMask( boundaryHandlers_, uid ); }

   inline BoundaryUID getBoundaryUID( const FlagUID & flag ) const;
   inline BoundaryUID getBoundaryUID( const flag_t    flag ) const;

   inline shared_ptr<BoundaryConfiguration> createBoundaryConfiguration( const BoundaryUID & uid, const Config::BlockHandle & config ) const
            { return createBoundaryConfiguration( boundaryHandlers_, uid, config ); }

   inline bool checkConsistency( const uint_t numberOfGhostLayersToInclude = 0 ) const;
   inline bool checkConsistency( const CellInterval & cells ) const;

   inline void refresh( const uint_t numberOfGhostLayersToInclude = 0 ); // reset near boundary ...
   inline void refresh( const CellInterval & cells );                    // ... flags for all handlers

   inline void refreshOutermostLayer( cell_idx_t thickness = 1 ); // reset near boundary flags in the outermost "inner" layers

   //** General Flag Handling ******************************************************************************************
   /*! \name General Flag Handling */
   //@{
   inline void setFlag( const FlagUID & flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                        const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );
   inline void setFlag( const flag_t    flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                        const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );

   inline void setFlag( const FlagUID & flag, const CellInterval & cells,
                        const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );
   inline void setFlag( const flag_t    flag, const CellInterval & cells,
                        const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );

   template< typename CellIterator >
   inline void setFlag( const FlagUID & flag, const CellIterator & begin, const CellIterator & end,
                        const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );
   template< typename CellIterator >
   inline void setFlag( const flag_t    flag, const CellIterator & begin, const CellIterator & end,
                        const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );

   inline void forceFlag( const FlagUID & flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                          const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );
   inline void forceFlag( const flag_t    flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                          const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );

   inline void forceFlag( const FlagUID & flag, const CellInterval & cells,
                          const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );
   inline void forceFlag( const flag_t    flag, const CellInterval & cells,
                          const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );

   template< typename CellIterator >
   inline void forceFlag( const FlagUID & flag, const CellIterator & begin, const CellIterator & end,
                          const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );
   template< typename CellIterator >
   inline void forceFlag( const flag_t    flag, const CellIterator & begin, const CellIterator & end,
                          const BoundaryConfiguration & parameter = BoundaryConfiguration::null() );

   inline void removeFlag( const FlagUID & flag, const uint_t numberOfGhostLayersToInclude = 0 );
   inline void removeFlag( const flag_t    flag, const uint_t numberOfGhostLayersToInclude = 0 );

   inline void removeFlag( const FlagUID & flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z );
   inline void removeFlag( const flag_t    flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z );

   inline void removeFlag( const FlagUID & flag, const CellInterval & cells );
   inline void removeFlag( const flag_t    flag, const CellInterval & cells );

   template< typename CellIterator >
   inline void removeFlag( const FlagUID & flag, const CellIterator & begin, const CellIterator & end );
   template< typename CellIterator >
   inline void removeFlag( const flag_t    flag, const CellIterator & begin, const CellIterator & end );
   //@}
   //*******************************************************************************************************************

   //** Clear Cells ****************************************************************************************************
   /*! \name Clear Cells */
   //@{
   inline void clear( const uint_t numberOfGhostLayersToInclude = 0 );
   inline void clear( const cell_idx_t x, const cell_idx_t y, const cell_idx_t z );
   inline void clear( const CellInterval & cells );
   template< typename CellIterator >
   inline void clear( const CellIterator & begin, const CellIterator & end );
   //@}
   //*******************************************************************************************************************

   //** Boundary Treatment *********************************************************************************************
   /*! \name Boundary Treatment */
   //@{
   static BlockSweep getBlockSweep( const BlockDataID handling, const uint_t numberOfGhostLayersToInclude = 0 )
      { return BlockSweep( handling, numberOfGhostLayersToInclude ); }

   inline void operator()( const uint_t numberOfGhostLayersToInclude = 0 );
   inline void operator()( const cell_idx_t x, const cell_idx_t y, const cell_idx_t z );
   inline void operator()( const CellInterval & cells );
   template< typename CellIterator >
   inline void operator()( const CellIterator & begin, const CellIterator & end );

   inline void beforeBoundaryTreatment() { beforeBoundaryTreatment( boundaryHandlers_ ); }
   inline void  afterBoundaryTreatment() {  afterBoundaryTreatment( boundaryHandlers_ ); }
   //@}
   //*******************************************************************************************************************

   //** Pack / Unpack boundary handling collection *********************************************************************
   /*! \name Pack / Unpack boundary handling collection */
   //@{
   template< typename Buffer_T >
   void   pack( Buffer_T & buffer, stencil::Direction direction, const uint_t numberOfLayers = 1, const bool assumeIdenticalFlagMapping = true ) const;
   template< typename Buffer_T >
   void unpack( Buffer_T & buffer, stencil::Direction direction, const uint_t numberOfLayers = 1, const bool assumeIdenticalFlagMapping = true );
   //@}
   //*******************************************************************************************************************

   inline void        toStream( std::ostream & os ) const;
   inline std::string toString() const;

private:

   CellInterval getGhostLayerCellInterval( const uint_t numberOfGhostLayersToInclude ) const;

   template< typename Head, typename Tail >
   inline bool isEmpty( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z ) const;
   inline bool isEmpty( const boost::tuples::null_type &, const cell_idx_t, const cell_idx_t, const cell_idx_t ) const { return true; }

   template< typename Head, typename Tail >
   inline bool isEmpty( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const ConstFlagFieldBaseIterator & it ) const;
   inline bool isEmpty( const boost::tuples::null_type &, const ConstFlagFieldBaseIterator & ) const { return true; }

   template< typename Head, typename Tail >
   inline bool consideredByAllHandlers( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z ) const;
   inline bool consideredByAllHandlers( const boost::tuples::null_type &, const cell_idx_t, const cell_idx_t, const cell_idx_t ) const { return true; }

   template< typename Head, typename Tail >
   inline bool consideredByAllHandlers( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const ConstFlagFieldBaseIterator & it ) const;
   inline bool consideredByAllHandlers( const boost::tuples::null_type &, const ConstFlagFieldBaseIterator & ) const { return true; }

   //** Get Boundary Handling (private helper functions) ***************************************************************
   /*! \name Get Boundary Handling (private helper functions) */
   //@{

   // matching type (-> BoundaryHandling_T) not yet found ...

   template< typename BoundaryHandling_T, typename Head, typename Tail >
   inline const BoundaryHandling_T & getBoundaryHandling( const BoundaryHandlingUID & uid, const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                          typename boost::enable_if< boost::is_same< BoundaryHandling_T, Head > >::type* /*dummy*/ = 0,
                                                          typename boost::enable_if< boost::is_same< typename boost::is_same< Tail, boost::tuples::null_type >::type,
                                                                                                     boost::false_type > >::type* /*dummy*/ = 0 ) const
   {
      if( uid == boundaryHandlers.get_head().getUID() )
         return boundaryHandlers.get_head();
      else
         return getBoundaryHandling_TypeExists< BoundaryHandling_T, typename Tail::head_type, typename Tail::tail_type >( uid, boundaryHandlers.get_tail() );

   }

   template< typename BoundaryHandling_T, typename Head, typename Tail >
   inline const BoundaryHandling_T & getBoundaryHandling( const BoundaryHandlingUID & uid, const boost::tuples::cons<Head, boost::tuples::null_type> & boundaryHandlers,
                                                          typename boost::enable_if< boost::is_same< BoundaryHandling_T, Head > >::type* /*dummy*/ = 0 ) const
   {
      if( uid == boundaryHandlers.get_head().getUID() )
         return boundaryHandlers.get_head();
      else
         WALBERLA_ABORT( "The requested boundary handler " << uid.getIdentifier() << " is not part of this boundary handling collection." );
   }

   template< typename BoundaryHandling_T, typename Head, typename Tail >
   inline const BoundaryHandling_T & getBoundaryHandling( const BoundaryHandlingUID & uid, const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                          typename boost::enable_if< boost::is_same< typename boost::is_same< BoundaryHandling_T, Head >::type,
                                                                                                     boost::false_type > >::type* /*dummy*/ = 0,
                                                          typename boost::enable_if< boost::is_same< typename boost::is_same< Tail, boost::tuples::null_type >::type,
                                                                                                     boost::false_type > >::type* /*dummy*/ = 0 ) const
   {
      return getBoundaryHandling< BoundaryHandling_T, typename Tail::head_type, typename Tail::tail_type >( uid, boundaryHandlers.get_tail() );
   }

   template< typename BoundaryHandling_T, typename Head, typename Tail >
   inline const BoundaryHandling_T & getBoundaryHandling( const BoundaryHandlingUID & /*uid*/, const boost::tuples::cons<Head, boost::tuples::null_type> & /*boundaryHandlers*/,
                                                          typename boost::enable_if< boost::is_same< typename boost::is_same< BoundaryHandling_T, Head >::type,
                                                                                                     boost::false_type > >::type* /*dummy*/ = 0 ) const
   {
      static_assert( sizeof(BoundaryHandling_T) == 0, "The requested boundary handling is not part of this boundary handling collection." );
   }

   //template< typename BoundaryHandling_T >
   //inline const BoundaryHandling_T & getBoundaryHandling( const BoundaryHandlingUID & /*uid*/, const boost::tuples::null_type & ) const
   //{
   //   static_assert( sizeof(BoundaryHandling_T) == 0, "The requested boundary handling is not part of this boundary handling collection." );
   //}

   // matching type (-> BoundaryHandling_T) exists!

   template< typename BoundaryHandling_T, typename Head, typename Tail >
   inline const BoundaryHandling_T & getBoundaryHandling_TypeExists( const BoundaryHandlingUID & uid, const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                     typename boost::enable_if< boost::is_same< BoundaryHandling_T, Head > >::type* /*dummy*/ = 0,
                                                                     typename boost::enable_if< boost::is_same< typename boost::is_same< Tail, boost::tuples::null_type >::type,
                                                                                                                boost::false_type > >::type* /*dummy*/ = 0 ) const
   {
      if( uid == boundaryHandlers.get_head().getUID() )
         return boundaryHandlers.get_head();
      else
         return getBoundaryHandling_TypeExists< BoundaryHandling_T, typename Tail::head_type, typename Tail::tail_type >( uid, boundaryHandlers.get_tail() );

   }

   template< typename BoundaryHandling_T, typename Head, typename Tail >
   inline const BoundaryHandling_T & getBoundaryHandling_TypeExists( const BoundaryHandlingUID & uid, const boost::tuples::cons<Head, boost::tuples::null_type> & boundaryHandlers,
                                                                     typename boost::enable_if< boost::is_same< BoundaryHandling_T, Head > >::type* /*dummy*/ = 0 ) const
   {
      if( uid == boundaryHandlers.get_head().getUID() )
         return boundaryHandlers.get_head();
      else
         WALBERLA_ABORT( "The requested boundary handler " << uid.getIdentifier() << " is not part of this boundary handling collection." );
   }

   template< typename BoundaryHandling_T, typename Head, typename Tail >
   inline const BoundaryHandling_T & getBoundaryHandling_TypeExists( const BoundaryHandlingUID & uid, const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                     typename boost::enable_if< boost::is_same< typename boost::is_same< BoundaryHandling_T, Head >::type,
                                                                                                                boost::false_type > >::type* /*dummy*/ = 0,
                                                                     typename boost::enable_if< boost::is_same< typename boost::is_same< Tail, boost::tuples::null_type >::type,
                                                                                                                boost::false_type > >::type* /*dummy*/ = 0 ) const
   {
      return getBoundaryHandling_TypeExists< BoundaryHandling_T, typename Tail::head_type, typename Tail::tail_type >( uid, boundaryHandlers.get_tail() );
   }

   template< typename BoundaryHandling_T, typename Head, typename Tail >
   inline const BoundaryHandling_T & getBoundaryHandling_TypeExists( const BoundaryHandlingUID & uid, const boost::tuples::cons<Head, boost::tuples::null_type> & /*boundaryHandlers*/,
                                                                     typename boost::enable_if< boost::is_same< typename boost::is_same< BoundaryHandling_T, Head >::type,
                                                                                                                boost::false_type > >::type* /*dummy*/ = 0 ) const
   {
      WALBERLA_ABORT( "The requested boundary handler " << uid.getIdentifier() << " is not part of this boundary handling collection." );
   }

   //template< typename BoundaryHandling_T >
   //inline const BoundaryHandling_T & getBoundaryHandling_TypeExists( const BoundaryHandlingUID & uid, const boost::tuples::null_type & ) const
   //{
   //   WALBERLA_ABORT( "The requested boundary handler " << uid.getIdentifier() << " is not part of this boundary handling collection." );
   //}
   //@}
   //*******************************************************************************************************************

   template< typename Head, typename Tail >
   inline void checkForUniqueBoundaryHandlingUIDs( const boost::tuples::cons<Head, Tail> & boundaryHandlers ) const;
   inline void checkForUniqueBoundaryHandlingUIDs( const boost::tuples::null_type & ) const {}

   inline std::vector< BoundaryUID > getBoundaryUIDs() const;
   template< typename Head, typename Tail >
   inline void getBoundaryUIDs( const boost::tuples::cons<Head, Tail> & boundaryHandlers, std::vector< BoundaryUID > & uids ) const;
   inline void getBoundaryUIDs( const boost::tuples::null_type &, std::vector< BoundaryUID > & ) const {}

   template< typename Head, typename Tail >
   inline bool checkForIdenticalFlagFields( const boost::tuples::cons<Head, Tail> & boundaryHandlers ) const;
   inline bool checkForIdenticalFlagFields( const boost::tuples::null_type & ) const { return true; }

   inline uint_t numberOfMatchingBoundaryHandlers( const BoundaryHandlingUID & uid ) const;

   template< typename Head, typename Tail >
   inline uint_t numberOfMatchingBoundaryHandlers( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const BoundaryHandlingUID & uid ) const;
   inline uint_t numberOfMatchingBoundaryHandlers( const boost::tuples::null_type &, const BoundaryHandlingUID & ) const { return uint_c(0); }

   template< typename Head, typename Tail >
   inline uint_t numberOfMatchingHandlers( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag ) const;
   inline uint_t numberOfMatchingHandlers( const boost::tuples::null_type &, const flag_t ) const { return uint_c(0); }

   template< typename Head, typename Tail >
   inline uint_t numberOfMatchingHandlersForDomain( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag ) const;
   inline uint_t numberOfMatchingHandlersForDomain( const boost::tuples::null_type &, const flag_t ) const { return uint_c(0); }

   template< typename Head, typename Tail >
   inline uint_t numberOfMatchingHandlersForBoundary( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag ) const;
   inline uint_t numberOfMatchingHandlersForBoundary( const boost::tuples::null_type &, const flag_t ) const { return uint_c(0); }

   template< typename Head, typename Tail >
   inline bool containsBoundaryCondition( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const BoundaryUID & uid ) const;
   inline bool containsBoundaryCondition( const boost::tuples::null_type &, const BoundaryUID & ) const { return false; }

   template< typename Head, typename Tail >
   inline bool containsBoundaryCondition( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag ) const;
   inline bool containsBoundaryCondition( const boost::tuples::null_type &, const flag_t ) const { return false; }

   template< typename Head, typename Tail >
   inline flag_t getBoundaryMask( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const BoundaryUID & uid ) const;
   inline flag_t getBoundaryMask( const boost::tuples::null_type &, const BoundaryUID & ) const { return numeric_cast<flag_t>(0); }

   template< typename Head, typename Tail >
   inline BoundaryUID getBoundaryUID( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag ) const;
   inline BoundaryUID getBoundaryUID( const boost::tuples::null_type &, const flag_t ) const;

   template< typename Head, typename Tail >
   inline shared_ptr<BoundaryConfiguration> createBoundaryConfiguration( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                         const BoundaryUID & uid, const Config::BlockHandle & config ) const;
   inline shared_ptr<BoundaryConfiguration> createBoundaryConfiguration( const boost::tuples::null_type &, const BoundaryUID &,
                                                                         const Config::BlockHandle & ) const
                                                                                  { WALBERLA_ASSERT( false ); return make_shared<BoundaryConfiguration>(); }

   template< typename Head, typename Tail >
   inline bool checkConsistency( const boost::tuples::cons<Head, Tail> & boundaryHandlers, const CellInterval & cells ) const;
   inline bool checkConsistency( const boost::tuples::null_type &, const CellInterval & ) const { return true; }

   template< typename Head, typename Tail >
   inline void refresh(       boost::tuples::cons<Head, Tail> & boundaryHandlers, const CellInterval & cells );
   inline void refresh( const boost::tuples::null_type &, const CellInterval & ) const {}

   template< typename Head, typename Tail >
   inline void refreshOutermostLayer(       boost::tuples::cons<Head, Tail> & boundaryHandlers, cell_idx_t thickness );
   inline void refreshOutermostLayer( const boost::tuples::null_type &, cell_idx_t ) const {}

   //** General Flag Handling (private helper functions) ***************************************************************
   /*! \name General Flag Handling (private helper functions) */
   //@{
   template< typename Head, typename Tail >
          void setFlag( boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag,
                        const cell_idx_t x, const cell_idx_t y, const cell_idx_t z, const BoundaryConfiguration & parameter );
   inline void setFlag( const boost::tuples::null_type &, const flag_t flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                        const BoundaryConfiguration & );

   template< typename Head, typename Tail >
          void setFlag( boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag,
                        const CellInterval & cells, const BoundaryConfiguration & parameter );
   inline void setFlag( const boost::tuples::null_type &, const flag_t flag, const CellInterval & cells, const BoundaryConfiguration & );

   void forceFlagHelper( const flag_t flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z, const BoundaryConfiguration & parameter );

   template< typename Head, typename Tail >
          flag_t flagsToRemove( boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag,
                                const cell_idx_t x, const cell_idx_t y, const cell_idx_t z );
   inline flag_t flagsToRemove( const boost::tuples::null_type &, const flag_t, const cell_idx_t, const cell_idx_t, const cell_idx_t ) const { return 0; }

   template< typename Head, typename Tail >
          void removeFlag( boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag,
                           const cell_idx_t x, const cell_idx_t y, const cell_idx_t z );
   inline void removeFlag( const boost::tuples::null_type &, const flag_t flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z );
   //@}
   //*******************************************************************************************************************

   //** Clear Cells (private helper functions) *************************************************************************
   /*! \name Clear Cells (private helper functions) */
   //@{
   template< typename Head, typename Tail >
          flag_t clear( boost::tuples::cons<Head, Tail> & boundaryHandlers, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z );
   inline flag_t clear( const boost::tuples::null_type &, const cell_idx_t, const cell_idx_t, const cell_idx_t ) const { return 0; }
   //@}
   //*******************************************************************************************************************

   //** Boundary Treatment (private helper functions) ******************************************************************
   /*! \name Boundary Treatment (private helper functions) */
   //@{
   template< typename Head, typename Tail >
   inline void execute( boost::tuples::cons<Head, Tail> & boundaryHandlers, const uint_t numberOfGhostLayersToInclude );
   inline void execute( const boost::tuples::null_type &, const uint_t ) const {}

   template< typename Head, typename Tail >
   inline void execute( boost::tuples::cons<Head, Tail> & boundaryHandlers, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z );
   inline void execute( const boost::tuples::null_type &, const cell_idx_t, const cell_idx_t, const cell_idx_t ) const {}

   template< typename Head, typename Tail >
   inline void execute( boost::tuples::cons<Head, Tail> & boundaryHandlers, const CellInterval & cells );
   inline void execute( const boost::tuples::null_type &, const CellInterval & ) const {}

   template< typename CellIterator, typename Head, typename Tail >
   inline void execute( boost::tuples::cons<Head, Tail> & boundaryHandlers, const CellIterator & begin, const CellIterator & end );
   template< typename CellIterator >
   inline void execute( const boost::tuples::null_type &, const CellIterator &, const CellIterator & ) const {}

   template< typename Head, typename Tail >
   inline void beforeBoundaryTreatment( boost::tuples::cons<Head, Tail> & boundaryHandlers );
   inline void beforeBoundaryTreatment( const boost::tuples::null_type & ) const {}

   template< typename Head, typename Tail >
   inline void  afterBoundaryTreatment( boost::tuples::cons<Head, Tail> & boundaryHandlers );
   inline void  afterBoundaryTreatment( const boost::tuples::null_type & ) const {}
   //@}
   //*******************************************************************************************************************

   //** Pack / Unpack boundary handling (private helper functions) *****************************************************
   /*! \name Pack / Unpack boundary handling (private helper functions) */
   //@{
   std::map< std::string, flag_t > getFlagMapping() const { return boundaryHandlers_.get_head().getFlagMapping(); }

   template< typename Buffer_T >
   std::vector< flag_t > getNeighborFlagMapping( Buffer_T & buffer, const bool assumeIdenticalFlagMapping, bool & identicalFlagMapping ) const
   {
      return boundaryHandlers_.get_head().getNeighborFlagMapping( buffer, assumeIdenticalFlagMapping, identicalFlagMapping );
   }

   inline void translateMask( flag_t & mask, const std::vector< flag_t > & flagMapping ) const;

   inline CellInterval   getPackingInterval( stencil::Direction direction, const uint_t numberOfLayers ) const;
   inline CellInterval getUnpackingInterval( stencil::Direction direction, const uint_t numberOfLayers ) const;

   template< typename Head, typename Tail, typename Buffer_T >
   inline void pack( const boost::tuples::cons<Head, Tail> & boundaryHandlers, Buffer_T & buffer, const flag_t mask,
                     const cell_idx_t x, const cell_idx_t y, const cell_idx_t z ) const;
   template< typename Buffer_T >
   inline void pack( const boost::tuples::null_type &, Buffer_T &, const flag_t, const cell_idx_t, const cell_idx_t, const cell_idx_t ) const {}

   template< typename Head, typename Tail, typename Buffer_T >
   inline void unpack( boost::tuples::cons<Head, Tail> & boundaryHandlers, Buffer_T & buffer, const flag_t mask,
                       const cell_idx_t x, const cell_idx_t y, const cell_idx_t z );
   template< typename Buffer_T >
   inline void unpack( const boost::tuples::null_type &, Buffer_T &, const flag_t, const cell_idx_t, const cell_idx_t, const cell_idx_t ) const {}
   //@}
   //*******************************************************************************************************************

   template< typename Head, typename Tail >
   inline void toStream( const boost::tuples::cons<Head, Tail> & boundaryHandlers, std::ostream & os ) const;
   inline void toStream( const boost::tuples::null_type &, std::ostream & ) const {}



   const BoundaryHandlingCollectionUID uid_;

   FlagField_T * const flagField_;

   const CellInterval outerBB_;

   Tuple boundaryHandlers_;

}; // class BoundaryHandlingCollection



template< typename FlagField_T, typename Tuple >
BoundaryHandlingCollection< FlagField_T, Tuple >::BoundaryHandlingCollection( const std::string & identifier, FlagField_T * const flagField,
                                                                              const Tuple & boundaryHandlers ) :

   uid_( identifier ),
   flagField_( flagField ),
   outerBB_( -cell_idx_c( flagField_->nrOfGhostLayers() ), -cell_idx_c( flagField_->nrOfGhostLayers() ), -cell_idx_c( flagField_->nrOfGhostLayers() ),
             cell_idx_c( flagField_->xSize() + flagField_->nrOfGhostLayers() ) - 1, cell_idx_c( flagField_->ySize() + flagField_->nrOfGhostLayers() ) - 1,
             cell_idx_c( flagField_->zSize() + flagField_->nrOfGhostLayers() ) - 1 ),
   boundaryHandlers_( boundaryHandlers )
{
   if( flagField_->nrOfGhostLayers() < 1 )
      WALBERLA_ABORT( "The flag field passed to the boundary handling collection\"" << identifier << "\" must contain at least one ghost layer!" );

   if( !checkForIdenticalFlagFields( boundaryHandlers_ ) )
      WALBERLA_ABORT( "The flag field passed to the boundary handling collection\"" << identifier <<
                      "\" must be the same flag field that is registered at all boundary handlers!" );

   checkForUniqueBoundaryHandlingUIDs( boundaryHandlers_ );

   // check for unique boundaries
   std::vector< BoundaryUID > uids = getBoundaryUIDs();
   for( auto uid = uids.begin(); uid != uids.end(); ++uid )
      if( std::count( uids.begin(), uids.end(), *uid ) != 1 )
         WALBERLA_ABORT( "Every boundary condition registered at a boundary handler at the same boundary handling collection must have a unique boundary UID!\n"
                         "The boundary UID \"" << *uid << "\" is not unique for boundary handling collection \"" << uid_.getIdentifier() << "\"." );
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::isEmpty( const cell_idx_t x, const cell_idx_t y, const cell_idx_t z ) const
{
   WALBERLA_ASSERT( outerBB_.contains(x,y,z) );

   return isEmpty( boundaryHandlers_, x, y, z );
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::isEmpty( const ConstFlagFieldBaseIterator & it ) const
{
   WALBERLA_ASSERT_EQUAL( it.getField(), flagField_ );
   WALBERLA_ASSERT( outerBB_.contains(it.x(),it.y(),it.z()) );

   return isEmpty( boundaryHandlers_, it );
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::consideredByAllHandlers( const uint_t numberOfGhostLayersToInclude ) const
{
   CellInterval cells = getGhostLayerCellInterval( numberOfGhostLayersToInclude );
   return consideredByAllHandlers( cells );
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::consideredByAllHandlers( const cell_idx_t x, const cell_idx_t y, const cell_idx_t z ) const
{
   WALBERLA_ASSERT( outerBB_.contains(x,y,z) );

   return consideredByAllHandlers( boundaryHandlers_, x, y, z );
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::consideredByAllHandlers( const ConstFlagFieldBaseIterator & it ) const
{
   WALBERLA_ASSERT_EQUAL( it.getField(), flagField_ );
   WALBERLA_ASSERT( outerBB_.contains(it.x(),it.y(),it.z()) );

   return consideredByAllHandlers( boundaryHandlers_, it );
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::consideredByAllHandlers( const CellInterval & cells ) const
{
   WALBERLA_ASSERT( outerBB_.contains( cells ) );

   for( auto z = cells.zMin(); z <= cells.zMax(); ++z )
      for( auto y = cells.yMin(); y <= cells.yMax(); ++y )
         for( auto x = cells.xMin(); x <= cells.xMax(); ++x )
            if( !consideredByAllHandlers(x,y,z) ) return false;
   return true;
}



template< typename FlagField_T, typename Tuple >
template< typename CellIterator >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::consideredByAllHandlers( const CellIterator & begin, const CellIterator & end ) const
{
   for( auto cell = begin; cell != end; ++cell )
      if( !consideredByAllHandlers( cell->x(), cell->y(), cell->z() ) )
         return false;
   return true;
}



template< typename FlagField_T, typename Tuple >
template< typename BoundaryHandling_T >
inline const BoundaryHandling_T & BoundaryHandlingCollection< FlagField_T, Tuple >::getBoundaryHandling( const BoundaryHandlingUID & uid ) const
{
   return getBoundaryHandling< BoundaryHandling_T, typename Tuple::head_type, typename Tuple::tail_type >( uid, boundaryHandlers_ );
}



template< typename FlagField_T, typename Tuple >
template< typename BoundaryHandling_T >
inline BoundaryHandling_T & BoundaryHandlingCollection< FlagField_T, Tuple >::getBoundaryHandling( const BoundaryHandlingUID & uid )
{
   return const_cast< BoundaryHandling_T & >( static_cast< const BoundaryHandlingCollection * >( this )->template getBoundaryHandling< BoundaryHandling_T >( uid ) );
}



template< typename FlagField_T, typename Tuple >
inline uint_t BoundaryHandlingCollection< FlagField_T, Tuple >::numberOfMatchingHandlers( const flag_t flag ) const
{
   WALBERLA_ASSERT( field::isFlag(flag) );
   return numberOfMatchingHandlers( boundaryHandlers_, flag );
}



template< typename FlagField_T, typename Tuple >
inline uint_t BoundaryHandlingCollection< FlagField_T, Tuple >::numberOfMatchingHandlersForDomain( const flag_t flag ) const
{
   WALBERLA_ASSERT( field::isFlag(flag) );
   return numberOfMatchingHandlersForDomain( boundaryHandlers_, flag );
}



template< typename FlagField_T, typename Tuple >
inline uint_t BoundaryHandlingCollection< FlagField_T, Tuple >::numberOfMatchingHandlersForBoundary( const flag_t flag ) const
{
   WALBERLA_ASSERT( field::isFlag(flag) );
   return numberOfMatchingHandlersForBoundary( boundaryHandlers_, flag );
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::containsBoundaryCondition( const BoundaryUID & uid ) const
{
   return containsBoundaryCondition( boundaryHandlers_, uid );
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::containsBoundaryCondition( const FlagUID & flag ) const
{
   if( flagField_->flagExists( flag ) )
      return containsBoundaryCondition( flagField_->getFlag( flag ) );
   return false;
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::containsBoundaryCondition( const flag_t flag ) const
{
   return containsBoundaryCondition( boundaryHandlers_, flag );
}



template< typename FlagField_T, typename Tuple >
inline BoundaryUID BoundaryHandlingCollection< FlagField_T, Tuple >::getBoundaryUID( const FlagUID & flag ) const
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   return getBoundaryUID( flagField_->getFlag( flag ) );
}



template< typename FlagField_T, typename Tuple >
inline BoundaryUID BoundaryHandlingCollection< FlagField_T, Tuple >::getBoundaryUID( const flag_t flag ) const
{
   WALBERLA_ASSERT( field::isFlag( flag ) );
   WALBERLA_ASSERT( flagField_->isRegistered( flag ) );

   return getBoundaryUID( boundaryHandlers_, flag );
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::checkConsistency( const uint_t numberOfGhostLayersToInclude ) const
{
   CellInterval cells = getGhostLayerCellInterval( numberOfGhostLayersToInclude );
   return checkConsistency( cells );
}



template< typename FlagField_T, typename Tuple >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::checkConsistency( const CellInterval & cells ) const
{
   return checkConsistency( boundaryHandlers_, cells );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::refresh( const uint_t numberOfGhostLayersToInclude )
{
   CellInterval cells = getGhostLayerCellInterval( numberOfGhostLayersToInclude );
   refresh( cells );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::refresh( const CellInterval & cells )
{
   refresh( boundaryHandlers_, cells );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::refreshOutermostLayer( cell_idx_t thickness  )
{
   refreshOutermostLayer( boundaryHandlers_, thickness );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::setFlag( const FlagUID & flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                                                                       const BoundaryConfiguration & parameter )
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   setFlag( flagField_->getFlag( flag ), x, y, z, parameter );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::setFlag( const flag_t flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                                                                       const BoundaryConfiguration & parameter )
{
   if( !outerBB_.contains(x,y,z) )
      return;

   WALBERLA_ASSERT( !flagField_->isFlagSet( x, y, z, flag ) );

   setFlag( boundaryHandlers_, flag, x, y, z, parameter );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::setFlag( const FlagUID & flag, const CellInterval & cells,
                                                                       const BoundaryConfiguration & parameter )
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   setFlag( flagField_->getFlag( flag ), cells, parameter );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::setFlag( const flag_t flag, const CellInterval & cells,
                                                                       const BoundaryConfiguration & parameter )
{
   CellInterval localCells( outerBB_ );
   localCells.intersect( cells );

   if( localCells.empty() )
      return;

#ifndef NDEBUG
   for( auto cell = flagField_->beginSliceXYZ( localCells ); cell != flagField_->end(); ++cell )
      WALBERLA_ASSERT( !field::isFlagSet( cell, flag ) );
#endif

   setFlag( boundaryHandlers_, flag, localCells, parameter );
}



template< typename FlagField_T, typename Tuple >
template< typename CellIterator >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::setFlag( const FlagUID & flag, const CellIterator & begin, const CellIterator & end,
                                                                       const BoundaryConfiguration & parameter )
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   setFlag( flagField_->getFlag( flag ), begin, end, parameter );
}



template< typename FlagField_T, typename Tuple >
template< typename CellIterator >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::setFlag( const flag_t flag, const CellIterator & begin, const CellIterator & end,
                                                                       const BoundaryConfiguration & parameter )
{
   for( auto cell = begin; cell != end; ++cell )
      setFlag( flag, cell->x(), cell->y(), cell->z(), parameter );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::forceFlag( const FlagUID & flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                                                                         const BoundaryConfiguration & parameter )
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   forceFlag( flagField_->getFlag( flag ), x, y, z, parameter );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::forceFlag( const flag_t flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                                                                         const BoundaryConfiguration & parameter )
{
   if( !outerBB_.contains(x,y,z) )
      return;

   forceFlagHelper( flag, x, y, z, parameter );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::forceFlag( const FlagUID & flag, const CellInterval & cells,
                                                                         const BoundaryConfiguration & parameter )
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   forceFlag( flagField_->getFlag( flag ), cells, parameter );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::forceFlag( const flag_t flag, const CellInterval & cells,
                                                                         const BoundaryConfiguration & parameter )
{
   CellInterval localCells( outerBB_ );
   localCells.intersect( cells );

   if( localCells.empty() )
      return;

   for( auto z = localCells.zMin(); z <= localCells.zMax(); ++z )
      for( auto y = localCells.yMin(); y <= localCells.yMax(); ++y )
         for( auto x = localCells.xMin(); x <= localCells.xMax(); ++x )
            forceFlagHelper( flag, x, y, z, parameter );
}



template< typename FlagField_T, typename Tuple >
template< typename CellIterator >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::forceFlag( const FlagUID & flag, const CellIterator & begin, const CellIterator & end,
                                                                         const BoundaryConfiguration & parameter )
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   forceFlag( flagField_->getFlag( flag ), begin, end, parameter );
}



template< typename FlagField_T, typename Tuple >
template< typename CellIterator >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::forceFlag( const flag_t flag, const CellIterator & begin, const CellIterator & end,
                                                                         const BoundaryConfiguration & parameter )
{
   for( auto cell = begin; cell != end; ++cell )
      forceFlag( flag, cell->x(), cell->y(), cell->z(), parameter );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::removeFlag( const FlagUID & flag, const uint_t numberOfGhostLayersToInclude )
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   removeFlag( flagField_->getFlag( flag ), numberOfGhostLayersToInclude );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::removeFlag( const flag_t flag, const uint_t numberOfGhostLayersToInclude )
{
   CellInterval cells = getGhostLayerCellInterval( numberOfGhostLayersToInclude );
   removeFlag( flag, cells );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::removeFlag( const FlagUID & flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z )
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   removeFlag( flagField_->getFlag( flag ), x, y, z );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::removeFlag( const flag_t flag, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z )
{
   if( !outerBB_.contains(x,y,z) || !flagField_->isFlagSet( x, y, z, flag ) )
      return;

   removeFlag( boundaryHandlers_, flag, x, y, z );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::removeFlag( const FlagUID & flag, const CellInterval & cells )
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   removeFlag( flagField_->getFlag( flag ), cells );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::removeFlag( const flag_t flag, const CellInterval & cells )
{
   CellInterval localCells( outerBB_ );
   localCells.intersect( cells );

   if( localCells.empty() )
      return;

   for( auto z = localCells.zMin(); z <= localCells.zMax(); ++z )
      for( auto y = localCells.yMin(); y <= localCells.yMax(); ++y )
         for( auto x = localCells.xMin(); x <= localCells.xMax(); ++x )
            if( flagField_->isFlagSet( x, y, z, flag ) )
               removeFlag( boundaryHandlers_, flag, x, y, z );
}



template< typename FlagField_T, typename Tuple >
template< typename CellIterator >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::removeFlag( const FlagUID & flag, const CellIterator & begin, const CellIterator & end )
{
   WALBERLA_ASSERT( flagField_->flagExists( flag ) );

   removeFlag( flagField_->getFlag( flag ), begin, end );
}



template< typename FlagField_T, typename Tuple >
template< typename CellIterator >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::removeFlag( const flag_t flag, const CellIterator & begin, const CellIterator & end )
{
   for( auto cell = begin; cell != end; ++cell )
      removeFlag( flag, cell->x(), cell->y(), cell->z() );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::clear( const uint_t numberOfGhostLayersToInclude )
{
   CellInterval cells = getGhostLayerCellInterval( numberOfGhostLayersToInclude );
   clear( cells );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::clear( const cell_idx_t x, const cell_idx_t y, const cell_idx_t z )
{
   if( !outerBB_.contains(x,y,z) )
      return;

   flagField_->removeMask( x, y, z, clear( boundaryHandlers_, x, y, z ) );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::clear( const CellInterval & cells )
{
   CellInterval localCells( outerBB_ );
   localCells.intersect( cells );

   if( localCells.empty() )
      return;

   for( auto z = localCells.zMin(); z <= localCells.zMax(); ++z )
      for( auto y = localCells.yMin(); y <= localCells.yMax(); ++y )
         for( auto x = localCells.xMin(); x <= localCells.xMax(); ++x )
            flagField_->removeMask( x, y, z, clear( boundaryHandlers_, x, y, z ) );
}



template< typename FlagField_T, typename Tuple >
template< typename CellIterator >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::clear( const CellIterator & begin, const CellIterator & end )
{
   for( auto cell = begin; cell != end; ++cell )
      clear( cell->x(), cell->y(), cell->z() );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::operator()( const uint_t numberOfGhostLayersToInclude )
{
   execute( boundaryHandlers_, numberOfGhostLayersToInclude );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::operator()( const cell_idx_t x, const cell_idx_t y, const cell_idx_t z )
{
   execute( boundaryHandlers_, x, y, z );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::operator()( const CellInterval & cells )
{
   execute( boundaryHandlers_, cells );
}



template< typename FlagField_T, typename Tuple >
template< typename CellIterator >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::operator()( const CellIterator & begin, const CellIterator & end )
{
   execute( boundaryHandlers_, begin, end );
}



template< typename FlagField_T, typename Tuple >
template< typename Buffer_T >
void BoundaryHandlingCollection< FlagField_T, Tuple >::pack( Buffer_T & buffer, stencil::Direction direction, const uint_t numberOfLayers,
                                                             const bool assumeIdenticalFlagMapping ) const
{
#ifdef NDEBUG
   if( !assumeIdenticalFlagMapping )
#endif
      buffer << getFlagMapping();

   CellInterval interval = getPackingInterval( direction, numberOfLayers );

   for( auto z = interval.min()[2]; z <= interval.max()[2]; ++z ) {
      for( auto y = interval.min()[1]; y <= interval.max()[1]; ++y ) {
         for( auto x = interval.min()[0]; x <= interval.max()[0]; ++x )
         {
            const flag_t mask = flagField_->get(x,y,z);
            buffer << mask;
            pack( boundaryHandlers_, buffer, mask, x, y, z );
         }
      }
   }
}



template< typename FlagField_T, typename Tuple >
template< typename Buffer_T >
void BoundaryHandlingCollection< FlagField_T, Tuple >::unpack( Buffer_T & buffer, stencil::Direction direction, const uint_t numberOfLayers,
                                                               const bool assumeIdenticalFlagMapping )
{
   bool identicalFlagMapping = false;
   std::vector< flag_t > flagMapping = getNeighborFlagMapping( buffer, assumeIdenticalFlagMapping, identicalFlagMapping ); // neighbor-flag_t -> flag_t

   CellInterval interval = getUnpackingInterval( direction, numberOfLayers );
   clear( interval );

   for( auto z = interval.min()[2]; z <= interval.max()[2]; ++z ) {
      for( auto y = interval.min()[1]; y <= interval.max()[1]; ++y ) {
         for( auto x = interval.min()[0]; x <= interval.max()[0]; ++x )
         {
            flag_t mask;
            buffer >> mask;

            if( !identicalFlagMapping )
               translateMask( mask, flagMapping );

            (*flagField_)(x,y,z) = mask;
            unpack( boundaryHandlers_, buffer, mask, x, y, z );
         }
      }
   }
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::toStream( std::ostream & os ) const {

   os << "========================= BoundaryHandlingCollection =========================\n\n"
      << "Identifier: " << uid_.getIdentifier() << "\n\n"
      << "Included Boundary Handlers:\n\n";

   toStream( boundaryHandlers_, os );

   os << "\n========================= BoundaryHandlingCollection =========================\n";
}



template< typename FlagField_T, typename Tuple >
inline std::string BoundaryHandlingCollection< FlagField_T, Tuple >::toString() const {

   std::ostringstream oss;
   toStream( oss );
   return oss.str();
}



template< typename FlagField_T, typename Tuple >
CellInterval BoundaryHandlingCollection< FlagField_T, Tuple >::getGhostLayerCellInterval( const uint_t numberOfGhostLayersToInclude ) const
{
   CellInterval cells( -cell_idx_c( numberOfGhostLayersToInclude ),
                       -cell_idx_c( numberOfGhostLayersToInclude ),
                       -cell_idx_c( numberOfGhostLayersToInclude ),
                        cell_idx_c( flagField_->xSize() + numberOfGhostLayersToInclude ) - 1,
                        cell_idx_c( flagField_->ySize() + numberOfGhostLayersToInclude ) - 1,
                        cell_idx_c( flagField_->zSize() + numberOfGhostLayersToInclude ) - 1 );
   return cells;
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::isEmpty( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                       const cell_idx_t x, const cell_idx_t y, const cell_idx_t z ) const
{
   if( !(boundaryHandlers.get_head().isEmpty(x,y,z)) )
      return false;
   return isEmpty( boundaryHandlers.get_tail(), x, y, z );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::isEmpty( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                       const ConstFlagFieldBaseIterator & it ) const
{
   if( !(boundaryHandlers.get_head().isEmpty(it)) )
      return false;
   return isEmpty( boundaryHandlers.get_tail(), it );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::consideredByAllHandlers( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                         const cell_idx_t x, const cell_idx_t y, const cell_idx_t z ) const
{
   if( boundaryHandlers.get_head().isEmpty(x,y,z) )
      return false;
   return consideredByAllHandlers( boundaryHandlers.get_tail(), x, y, z );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::consideredByAllHandlers( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                         const ConstFlagFieldBaseIterator & it ) const
{
   if( boundaryHandlers.get_head().isEmpty(it) )
      return false;
   return consideredByAllHandlers( boundaryHandlers.get_tail(), it );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::checkForUniqueBoundaryHandlingUIDs( const boost::tuples::cons<Head, Tail> & boundaryHandlers ) const
{
   if( numberOfMatchingBoundaryHandlers( boundaryHandlers.get_head().getUID() ) != uint_c(1) )
      WALBERLA_ABORT( "Every boundary handler registered at the same boundary handling collection must have a unique boundary handling UID!\n"
                      "The boundary handling UID \"" << boundaryHandlers.get_head().getUID() << "\" is not unique for boundary handling collection \"" << uid_.getIdentifier() << "\"." );

   checkForUniqueBoundaryHandlingUIDs( boundaryHandlers.get_tail() );
}



template< typename FlagField_T, typename Tuple >
inline std::vector< BoundaryUID > BoundaryHandlingCollection< FlagField_T, Tuple >::getBoundaryUIDs() const
{
   std::vector< BoundaryUID > uids;
   getBoundaryUIDs( boundaryHandlers_, uids );
   return uids;
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::getBoundaryUIDs( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                               std::vector< BoundaryUID > & uids ) const
{
   std::vector< BoundaryUID > handlerUIDs = boundaryHandlers.get_head().getBoundaryUIDs();
   uids.insert( uids.end(), handlerUIDs.begin(), handlerUIDs.end() );
   getBoundaryUIDs( boundaryHandlers.get_tail(), uids );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::checkForIdenticalFlagFields( const boost::tuples::cons<Head, Tail> & boundaryHandlers ) const
{
   return checkForIdenticalFlagFields( boundaryHandlers.get_tail() ) &&
          boundaryHandlers.get_head().getFlagField() == flagField_ && boundaryHandlers.get_head().outerBB_ == outerBB_;
}



template< typename FlagField_T, typename Tuple >
inline uint_t BoundaryHandlingCollection< FlagField_T, Tuple >::numberOfMatchingBoundaryHandlers( const BoundaryHandlingUID & uid ) const
{
   return numberOfMatchingBoundaryHandlers( boundaryHandlers_, uid );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline uint_t BoundaryHandlingCollection< FlagField_T, Tuple >::numberOfMatchingBoundaryHandlers( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                                  const BoundaryHandlingUID & uid ) const
{
   return ( ( boundaryHandlers.get_head().getUID() == uid ) ? uint_c(1) : uint_c(0) ) +
          numberOfMatchingBoundaryHandlers( boundaryHandlers.get_tail(), uid );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline uint_t BoundaryHandlingCollection< FlagField_T, Tuple >::numberOfMatchingHandlers( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                          const flag_t flag ) const
{
   return ( ( (boundaryHandlers.get_head().getBoundaryMask() | boundaryHandlers.get_head().getDomainMask()) & flag ) == flag ? 1 : 0 ) +
          numberOfMatchingHandlers( boundaryHandlers.get_tail(), flag );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline uint_t BoundaryHandlingCollection< FlagField_T, Tuple >::numberOfMatchingHandlersForDomain( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                                   const flag_t flag ) const
{
   return ( ( boundaryHandlers.get_head().getDomainMask() & flag ) == flag ? 1 : 0 ) +
          numberOfMatchingHandlersForDomain( boundaryHandlers.get_tail(), flag );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline uint_t BoundaryHandlingCollection< FlagField_T, Tuple >::numberOfMatchingHandlersForBoundary( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                                     const flag_t flag ) const
{
   return ( ( boundaryHandlers.get_head().getBoundaryMask() & flag ) == flag ? 1 : 0 ) +
          numberOfMatchingHandlersForBoundary( boundaryHandlers.get_tail(), flag );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::containsBoundaryCondition( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                         const BoundaryUID & uid ) const
{
   if( boundaryHandlers.get_head().containsBoundaryCondition( uid ) )
      return true;
   return containsBoundaryCondition( boundaryHandlers.get_tail(), uid );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::containsBoundaryCondition( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                         const flag_t flag ) const
{
   if( boundaryHandlers.get_head().containsBoundaryCondition( flag ) )
      return true;
   return containsBoundaryCondition( boundaryHandlers.get_tail(), flag );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline typename BoundaryHandlingCollection< FlagField_T, Tuple >::flag_t
   BoundaryHandlingCollection< FlagField_T, Tuple >::getBoundaryMask( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                      const BoundaryUID & uid ) const
{
   if( boundaryHandlers.get_head().containsBoundaryCondition(uid) )
      return boundaryHandlers.get_head().getBoundaryMask(uid);
   return getBoundaryMask( boundaryHandlers.get_tail(), uid );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline BoundaryUID BoundaryHandlingCollection< FlagField_T, Tuple >::getBoundaryUID( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                     const flag_t flag ) const
{
   const Head & boundaryHandler = boundaryHandlers.get_head();

   if( boundaryHandler.containsBoundaryCondition( flag ) )
   {
      return boundaryHandler.getBoundaryUID( flag );
   }
   else
   {
      return getBoundaryUID( boundaryHandlers.get_tail(), flag );
   }
}



template< typename FlagField_T, typename Tuple >
inline BoundaryUID BoundaryHandlingCollection< FlagField_T, Tuple >::getBoundaryUID( const boost::tuples::null_type &, const flag_t flag ) const
{
   if( !flagField_->isRegistered( flag ) )
      WALBERLA_ABORT( "The requested flag with value " << flag << " is not registered at the flag field and is not handled "\
                      "by any boundary condition of boundary handling collection " << uid_.getIdentifier() << "!" );

   const FlagUID & flagUID = flagField_->getFlagUID( flag );
   WALBERLA_ABORT( "The requested flag " << flagUID.getIdentifier() << " is not handled by any boundary condition of "\
                   "boundary handling collection" << uid_.getIdentifier() << "!" );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline shared_ptr<BoundaryConfiguration>
   BoundaryHandlingCollection< FlagField_T, Tuple >::createBoundaryConfiguration( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                  const BoundaryUID & uid, const Config::BlockHandle & config ) const
{
   if( boundaryHandlers.get_head().containsBoundaryCondition(uid) )
      return boundaryHandlers.get_head().createBoundaryConfiguration( uid, config );
   return createBoundaryConfiguration( boundaryHandlers.get_tail(), uid, config );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline bool BoundaryHandlingCollection< FlagField_T, Tuple >::checkConsistency( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                const CellInterval & cells ) const
{
   return checkConsistency( boundaryHandlers.get_tail(), cells ) && boundaryHandlers.get_head().checkConsistency(cells);
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::refresh( boost::tuples::cons<Head, Tail> & boundaryHandlers, const CellInterval & cells )
{
   boundaryHandlers.get_head().refresh( cells );
   refresh( boundaryHandlers.get_tail(), cells );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::refreshOutermostLayer( boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                                     cell_idx_t thickness )
{
   boundaryHandlers.get_head().refreshOutermostLayer( thickness );
   refreshOutermostLayer( boundaryHandlers.get_tail(), thickness );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
void BoundaryHandlingCollection< FlagField_T, Tuple >::setFlag( boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag,
                                                                const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                                                                const BoundaryConfiguration & parameter )
{
   WALBERLA_ASSERT( outerBB_.contains(x,y,z) );

   Head & handler = boundaryHandlers.get_head();

   if( ( (handler.getBoundaryMask() | handler.getDomainMask()) & flag ) == flag )
   {
      flagField_->removeFlag( x, y, z, flag );
      handler.setFlag( flag, x, y, z, parameter );
   }

   setFlag( boundaryHandlers.get_tail(), flag, x, y, z, parameter );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::setFlag( const boost::tuples::null_type & /*boundaryHandlers*/, const flag_t flag,
                                                                       const cell_idx_t x, const cell_idx_t y, const cell_idx_t z,
                                                                       const BoundaryConfiguration & /*parameter*/ )
{
   WALBERLA_ASSERT( outerBB_.contains(x,y,z) );

   flagField_->addFlag( x, y, z, flag );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
void BoundaryHandlingCollection< FlagField_T, Tuple >::setFlag( boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag,
                                                                const CellInterval & cells, const BoundaryConfiguration & parameter )
{
   WALBERLA_ASSERT( outerBB_.contains(cells) );

   Head & handler = boundaryHandlers.get_head();

   if( ( (handler.getBoundaryMask() | handler.getDomainMask()) & flag ) == flag )
   {
      if( flagField_->isFlagSet( cells.xMin(), cells.yMin(), cells.zMin(), flag ) )
      {
         for( auto cell = flagField_->beginSliceXYZ( cells ); cell != flagField_->end(); ++cell )
            field::removeFlag( cell, flag );
      }

      handler.setFlag( flag, cells, parameter );
   }

   setFlag( boundaryHandlers.get_tail(), flag, cells, parameter );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::setFlag( const boost::tuples::null_type & /*boundaryHandlers*/, const flag_t flag,
                                                                       const CellInterval & cells, const BoundaryConfiguration & /*parameter*/ )
{
   WALBERLA_ASSERT( outerBB_.contains(cells) );

   for( auto cell = flagField_->beginSliceXYZ( cells ); cell != flagField_->end(); ++cell )
      field::addFlag( cell, flag );
}



template< typename FlagField_T, typename Tuple >
void BoundaryHandlingCollection< FlagField_T, Tuple >::forceFlagHelper( const flag_t flag, const cell_idx_t x, const cell_idx_t y,
                                                                        const cell_idx_t z, const BoundaryConfiguration & parameter )
{
   WALBERLA_ASSERT( outerBB_.contains(x,y,z) );

   flag_t mask = flagsToRemove( boundaryHandlers_, flag, x, y, z );

   WALBERLA_ASSERT( flagField_->isMaskSet( x, y, z, mask ) );

   static const flag_t digits = numeric_cast< flag_t >( std::numeric_limits< flag_t >::digits );
   for( flag_t bit = 0; bit < digits; ++bit )
   {
      flag_t flagToRemove = numeric_cast< flag_t >( static_cast<flag_t>(1) << bit );
      if( ( flagToRemove & mask ) == flagToRemove )
         removeFlag( boundaryHandlers_, flagToRemove, x, y, z );
   }

   setFlag( boundaryHandlers_, flag, x, y, z, parameter );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
typename BoundaryHandlingCollection< FlagField_T, Tuple >::flag_t
BoundaryHandlingCollection< FlagField_T, Tuple >::flagsToRemove( boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag,
                                                                 const cell_idx_t x, const cell_idx_t y, const cell_idx_t z )
{
   WALBERLA_ASSERT( outerBB_.contains(x,y,z) );

   const Head & handler = boundaryHandlers.get_head();
   flag_t mask = numeric_cast<flag_t>( handler.getBoundaryMask() | handler.getDomainMask() );

   if( ( mask & flag ) == flag )
      mask = numeric_cast<flag_t>( mask & flagField_->get(x,y,z) );
   else
      mask = numeric_cast<flag_t>(0);

   return numeric_cast<flag_t>( mask | flagsToRemove( boundaryHandlers.get_tail(), flag, x, y, z ) );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
void BoundaryHandlingCollection< FlagField_T, Tuple >::removeFlag( boost::tuples::cons<Head, Tail> & boundaryHandlers, const flag_t flag,
                                                                   const cell_idx_t x, const cell_idx_t y, const cell_idx_t z )
{
   WALBERLA_ASSERT( outerBB_.contains(x,y,z) );

   Head & handler = boundaryHandlers.get_head();

   if( ( (handler.getBoundaryMask() | handler.getDomainMask()) & flag ) == flag )
   {
      flagField_->addFlag( x, y, z, flag );
      handler.removeFlag( flag, x, y, z );
   }

   removeFlag( boundaryHandlers.get_tail(), flag, x, y, z );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::removeFlag( const boost::tuples::null_type & /*boundaryHandlers*/, const flag_t flag,
                                                                          const cell_idx_t x, const cell_idx_t y, const cell_idx_t z )
{
   WALBERLA_ASSERT( outerBB_.contains(x,y,z) );

   flagField_->removeFlag( x, y, z, flag );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
typename BoundaryHandlingCollection< FlagField_T, Tuple >::flag_t
BoundaryHandlingCollection< FlagField_T, Tuple >::clear( boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                         const cell_idx_t x, const cell_idx_t y, const cell_idx_t z )
{
   WALBERLA_ASSERT( outerBB_.contains(x,y,z) );

   const flag_t before = flagField_->get(x,y,z);
   boundaryHandlers.get_head().clear(x,y,z);
   const flag_t removedFlags = numeric_cast< flag_t >( before ^ flagField_->get(x,y,z) );
   flagField_->addMask( x, y, z, before );

   return numeric_cast< flag_t >( removedFlags | clear( boundaryHandlers.get_tail(), x, y, z ) );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::execute( boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                       const uint_t numberOfGhostLayersToInclude )
{
   boundaryHandlers.get_head()( numberOfGhostLayersToInclude );
   execute( boundaryHandlers.get_tail(), numberOfGhostLayersToInclude );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::execute( boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                       const cell_idx_t x, const cell_idx_t y, const cell_idx_t z )
{
   boundaryHandlers.get_head()(x,y,z);
   execute( boundaryHandlers.get_tail(), x, y, z );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::execute( boost::tuples::cons<Head, Tail> & boundaryHandlers, const CellInterval & cells )
{
   boundaryHandlers.get_head()( cells );
   execute( boundaryHandlers.get_tail(), cells );
}



template< typename FlagField_T, typename Tuple >
template< typename CellIterator, typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::execute( boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                       const CellIterator & begin, const CellIterator & end )
{
   boundaryHandlers.get_head()( begin, end );
   execute( boundaryHandlers.get_tail(), begin, end );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::beforeBoundaryTreatment( boost::tuples::cons<Head, Tail> & boundaryHandlers )
{
   boundaryHandlers.get_head().beforeBoundaryTreatment();
   beforeBoundaryTreatment( boundaryHandlers.get_tail() );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::afterBoundaryTreatment( boost::tuples::cons<Head, Tail> & boundaryHandlers )
{
   boundaryHandlers.get_head().afterBoundaryTreatment();
   afterBoundaryTreatment( boundaryHandlers.get_tail() );
}



template< typename FlagField_T, typename Tuple >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::translateMask( flag_t & mask, const std::vector< flag_t > & flagMapping ) const
{
   boundaryHandlers_.get_head().translateMask( mask, flagMapping );
}



template< typename FlagField_T, typename Tuple >
inline CellInterval BoundaryHandlingCollection< FlagField_T, Tuple >::getPackingInterval( stencil::Direction direction,
                                                                                          const uint_t numberOfLayers ) const
{
   return boundaryHandlers_.get_head().getPackingInterval( direction, numberOfLayers );
}



template< typename FlagField_T, typename Tuple >
inline CellInterval BoundaryHandlingCollection< FlagField_T, Tuple >::getUnpackingInterval( stencil::Direction direction,
                                                                                            const uint_t numberOfLayers ) const
{
   return boundaryHandlers_.get_head().getUnpackingInterval( direction, numberOfLayers );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail, typename Buffer_T >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::pack( const boost::tuples::cons<Head, Tail> & boundaryHandlers, Buffer_T & buffer,
                                                                    const flag_t mask, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z ) const
{
   boundaryHandlers_.get_head().pack( buffer, mask, x, y, z );
   pack( boundaryHandlers.get_tail(), buffer, mask, x, y, z );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail, typename Buffer_T >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::unpack( boost::tuples::cons<Head, Tail> & boundaryHandlers, Buffer_T & buffer,
                                                                      const flag_t mask, const cell_idx_t x, const cell_idx_t y, const cell_idx_t z )
{
   WALBERLA_ASSERT( outerBB_.contains(x,y,z) );

   Head & handler = boundaryHandlers.get_head();

   flag_t flag = (handler.getBoundaryMask() | handler.getDomainMask()) & mask;
   if( flag )
   {
      flagField_->removeFlag( x, y, z, flag );
      handler.unpack( buffer, flag, x, y, z );
   }

   unpack( boundaryHandlers.get_tail(), buffer, mask, x, y, z );
}



template< typename FlagField_T, typename Tuple >
template< typename Head, typename Tail >
inline void BoundaryHandlingCollection< FlagField_T, Tuple >::toStream( const boost::tuples::cons<Head, Tail> & boundaryHandlers,
                                                                        std::ostream & os ) const
{
   os << boundaryHandlers.get_head();
   toStream( boundaryHandlers.get_tail(), os );
}



} // namespace boundary



using boundary::BoundaryHandlingCollection;

template< typename FlagField_T, typename Tuple >
inline std::ostream & operator<<( std::ostream & os, const BoundaryHandlingCollection< FlagField_T, Tuple > & bhc ) {

   bhc.toStream( os );
   return os;
}



} // namespace walberla
